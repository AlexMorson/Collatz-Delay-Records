#include <fstream> // std::ifstream

#include <stdio.h> // printf
#include "ctpl.h" // ctpl::thread_pool

#include <string> // std::stoull, std::stoi
#include <queue> // std::queue
#include <vector> // std::vector
#include <utility> // std::pair
#include <mutex> // std::mutex, std::lock_guard
#include <condition_variable> // std::condition_variable
#include <thread> // std::this_thread::yield

typedef unsigned long long int collatz_t;
typedef unsigned int length_t;
typedef unsigned int block_t;
unsigned long long int MEM_SIZE = 100'000'000;

length_t getLength(collatz_t number) {
    length_t length = 0;

    // Odd or even
    while (!(number & 1)) {
        number >>= 1;
        length++;
    }

    // We have an odd number
    while (number != 1) {
        do {
            number += (number>>1) + 1; // 3n+1 and n/2
            length += 2;
        } while (number & 1);

        do { // n/2 steps until odd again
            number >>= 1;
            length++;
        } while (!(number & 1));
    }

    return length;
}

length_t getLength(collatz_t number, length_t* mem) {
    length_t length = 0;

    // Odd or even
    while (!(number & 1)){
        number >>= 1;
        length++;
    }

    // We have an odd number
    while (number != 1) {
        if (number < MEM_SIZE) {
            return mem[number>>1] + length;
        }

        do {
            number += (number>>1) + 1; // 3n+1 and n/2
            length += 2;
        } while (number & 1);

        do { // n/2 steps until odd again
            number >>= 1;
            length++;
        } while (!(number & 1));
    }

    return length;
}

void calcBlock(int threadId, collatz_t start, collatz_t end, block_t blockNum, length_t localRecord, length_t* mem, std::mutex* resultsMutex, std::vector<std::pair<block_t, std::queue<std::pair<collatz_t, length_t>>>>* results, std::condition_variable* resultsCondition) {
    std::queue<std::pair<collatz_t, length_t>> localResults;
    for (collatz_t num = start; num < end; num+=2) {
        length_t length = getLength(num, mem);
        if (length > localRecord) {
            localRecord = length;
            localResults.emplace(num, length);
        }
    }

    std::lock_guard<std::mutex> lock(*resultsMutex);
    results->push_back(std::pair<block_t, std::queue<std::pair<collatz_t, length_t>>>(blockNum, localResults));
    resultsCondition->notify_one();
}

void pushBlock(ctpl::thread_pool& threadPool, block_t& currentBlock, collatz_t blockSize, length_t globalRecord, length_t* mem, std::mutex& resultsMutex, std::vector<std::pair<block_t, std::queue<std::pair<collatz_t, length_t>>>>& results, std::condition_variable& resultsCondition) {
    threadPool.push(calcBlock, currentBlock*blockSize+1, (currentBlock+1)*blockSize, currentBlock, globalRecord, mem, &resultsMutex, &results, &resultsCondition);
    currentBlock++;
}

void memoBlock(int threadId, collatz_t start, collatz_t end, length_t* mem) {
    for (collatz_t i=start; i < end; i+=2) {
        mem[i>>1] = getLength(i);
    }
}

length_t* fillMem(ctpl::thread_pool& threadPool) {
    unsigned long long int memBlockSize = 1'000'000;
    unsigned int blockNum = MEM_SIZE / memBlockSize;
    length_t* mem = new length_t[MEM_SIZE>>1];

    for (unsigned int block=0; block < blockNum; ++block) {
        threadPool.push(memoBlock, block * memBlockSize + 1, (block + 1) * memBlockSize, mem);
    }
    threadPool.push(memoBlock, blockNum * memBlockSize + 1, MEM_SIZE, mem);

    while (threadPool.n_idle() != threadPool.size()) {
        std::this_thread::yield();
    }

    return mem;
}

int main(int argc, char* argv[]) {

    unsigned int threadNum = 16;
    unsigned int startingJobs = 20;
    collatz_t blockSize = 1'000'000;

    if (argc >= 2) {
        std::ifstream configFile(argv[1]);

        std::string threadNumString;
        std::string startingJobsString;
        std::string blockSizeString;
        std::string memSizeString;

        configFile >> threadNumString >> startingJobsString >> blockSizeString >> memSizeString;

        threadNum = std::stoi(threadNumString);
        startingJobs = std::stoi(startingJobsString);
        blockSize = std::stoull(blockSizeString);
        MEM_SIZE = std::stoull(memSizeString);
    } else {
        printf("Note: You can use the config file by running `multi config.txt`.\n");
    }

    printf("Using %u threads.\nStarting with %u jobs.\nCalculating %llu lengths in a batch.\nStoring up to %llu numbers in RAM.\n", threadNum, startingJobs, blockSize, MEM_SIZE);

    ctpl::thread_pool p(threadNum);

    printf("Starting memoisation.\n");
    length_t* mem = fillMem(p);
    printf("Finished memoisation.\n");

    block_t currentPushingBlock = 0;

    length_t globalRecord = 0;
    collatz_t globalRecordNum = 1;
    std::vector<std::pair<collatz_t, length_t>> globalRecords;

    std::vector<std::pair<block_t, std::queue<std::pair<collatz_t, length_t>>>> results;
    std::mutex resultsMutex;
    std::condition_variable resultsCondition;

    for (unsigned int i=0; i < startingJobs; i++) {
        pushBlock(p, currentPushingBlock, blockSize, globalRecord, mem, resultsMutex, results, resultsCondition);
    }

    unsigned int lastResultsSize = 0;
    block_t currentReadingBlock = 0;
    while (1) {
        std::unique_lock<std::mutex> resultsLock(resultsMutex);
        while (results.size() == lastResultsSize) {
            resultsCondition.wait(resultsLock);
        }
        while (results.size() > lastResultsSize) {
            if (results[lastResultsSize].first == currentReadingBlock) {
                lastResultsSize = results.size();

                std::sort(results.begin(), results.end(), [](std::pair<block_t, std::queue<std::pair<collatz_t, length_t>>> a, std::pair<block_t, std::queue<std::pair<collatz_t, length_t>>> b) {
                    return a.first < b.first;
                });

                while (results[0].first == currentReadingBlock) { // Next block in order
                    while (!results[0].second.empty()) { // Update global records
                        std::pair<collatz_t, length_t> record = results[0].second.front();
                        results[0].second.pop();

                        while (record.first > 2*globalRecordNum) {
                            ++globalRecord;
                            globalRecordNum <<=1;
                            globalRecords.emplace_back(globalRecordNum, globalRecord);
                            printf("1Record: %llu with length %u\n", globalRecordNum, globalRecord);
                        }
                        if (record.second > globalRecord) {
                            globalRecord = record.second;
                            globalRecordNum = record.first;
                            globalRecords.push_back(record);
                            printf("2Record: %llu with length %u\n", record.first, record.second);
                        }
                    }
                    results.erase(results.begin());
                    --lastResultsSize;

                    pushBlock(p, currentPushingBlock, blockSize, globalRecord, mem, resultsMutex, results, resultsCondition);

                    while (currentReadingBlock * blockSize > 2*globalRecordNum) {
                        ++globalRecord;
                        globalRecordNum <<=1;
                        globalRecords.emplace_back(globalRecordNum, globalRecord);
                        printf("3Record: %llu with length %u\n", globalRecordNum, globalRecord);
                    }

                    ++currentReadingBlock;
                }
            } else {
                ++lastResultsSize;
            }
        }
    }

    p.stop(true);

    /*while (!results[0].second.empty()) {
        std::pair<collatz_t, length_t> record(results[0].second.front());
        results[0].second.pop();

        printf("Number: %llu, Length: %u\n", record.first, record.second);
    }*/


    return 0;
}
