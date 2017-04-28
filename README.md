# Collatz Delay Checkers!!
These programs check for Collatz sequence delay records, where the **Delay** of a number is the number of steps needed to reach 1. For example D(3) = 7 because 3 -> 10 -> 5 -> 16 -> 8 -> 4 -> 2 -> 1, which is 7 steps.  
A positive integer N is called a **Delay Record** if for all M < N we have D(M) < D(N).

## Length
This is the original program.
* Memoisation:
  * Only includes odd numbers
  * Used when the number falls below a certain value
* Calculation:
  * Only odd numbers are checked for records
  * 3n+1 steps are combined with an n/2 step immediately

## Multi
The second version!
* Memoisation:
  * Again, only odd numbers
  * Calculated with many threads
* Calculation:
  * Threads are assigned jobs of ~1,000,000 numbers to check and return a queue of records
  * These may not actually be records - because each thread cannot know what the other threads are doing - so the main thread checks each one for validity
  * Only odd numbers are checked
* Implementation:
  * Uses ctpl::thread_pool to handle jobs (which in turn uses boost::lockfree::queue)

## Skip
Numero trois!
* Memoisation:
  * Stores the result of applying f(n) k times to the numbers (0..2^k-1) (Array d)
  * And the number of odd numbers encountered along the way (Array c)
* Calculation:
  * Uses the formula f^k(a\*2^k + b) = a\*3^c[b] + d[b] to perform k+c[b] steps at once
  * This will create an overestimate a lot of the time, so any records are checked for validity
