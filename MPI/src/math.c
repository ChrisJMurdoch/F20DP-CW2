
// Author: Christopher Murdoch
// Written for CW1 and reused for CW2

#include "../include/math.h"

#include <stdbool.h>

// Variables used for prime cache
#define PRIME_CACHE_KB 100
bool primeCache[PRIME_CACHE_KB*1024];
long const nPrimeCache = PRIME_CACHE_KB*1024;

/**
 * Use Sieve of Eratosthenes method to cache primes up to a certain number
 */
void initPrimeCache()
{
    // Initially set all numbers to prime
    for (int i=0; i<nPrimeCache; i++)
        primeCache[i] = true;
    
    // Set non-primes using Sieve of Eratosthenes method
    for (int i=2; i<nPrimeCache; i++)
        for (int d=i*2; d<nPrimeCache; d+=i)
            primeCache[d] = false;

    // Set 0 and 1 exceptions
    primeCache[0] = false;
    primeCache[1] = false;
}

/**
 * Find the greatest common factor of two given numbers
 * @param a First number
 * @param b Second number
 * @return Greatest common factor of a and b
 */
long gcf(long a, long b)
{
    // If the greater number is cached as a prime
    if ( (a>b && a<nPrimeCache && primeCache[a]) || (b>a && b<nPrimeCache && primeCache[b]) )
        return 1;
    
    // Calculate GCF
    while (b)
    {
        long temp = a % b;
        a = b;
        b = temp;
    }
    return a;
}

/**
 * Calculate the Euler Totient for a given number
 * @param n Number for which to calculate the Euler Totient
 * @return Euler Totient of n
 */
long eulerTotient(long n)
{
    long count = 0;
    for (long i=1; i<n; i++)
        count += gcf(n, i)==1;
    return count;
}
