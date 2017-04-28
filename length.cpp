#include <stdio.h>
#include <time.h>

typedef unsigned long long int collatz_t;
typedef unsigned int length_t;
unsigned int MEM_SIZE = 100'000'000;

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

length_t* fillMem() {
    length_t* mem = new length_t[MEM_SIZE>>1];
    for (unsigned int i=1; i < MEM_SIZE; ++i) {
        if (i & 1) {
            mem[i>>1] = getLength(i);
        }
    }
    return mem;
}

int main(int argc, char const* argv[]) {

    printf("Starting memoisation.\n");
    length_t* mem = fillMem();
    printf("Finished memoisation.\n");

    collatz_t number = 1;
    length_t maxLength = 0;

    while (1) {
        length_t length = getLength(number, mem);
        if (length > maxLength) {
            maxLength = length;
            printf("Number: %llu, Length: %u\n", number, length);
        }
        number++;
    }

    return 0;
}
