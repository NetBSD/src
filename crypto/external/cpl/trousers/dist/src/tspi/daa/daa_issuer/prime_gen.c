/*
 * Licensed Materials - Property of IBM
 *
 * trousers - An open source TCG Software Stack
 *
 * (C) Copyright International Business Machines Corp. 2004, 2005
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "bi.h"
#include "list.h"
#include "tsplog.h"

static unsigned long *primes;
static int primes_length;

/* Generates a random number of bit_length bit length. The first two bits and the last bit of this
 * number are always set, therefore the number is odd and >= (2^(bit_length-1)+2^(bit_length-2)+1)
 *
 * bit_length: The length of the number to be generated, in bits
 * return: a random number of bitLength bit length with first and last bits set
 */
void
random_odd_bi(bi_ptr bi, int bit_length)
{
	if (bit_length > 0) {
		bi_urandom(bi, bit_length);
		//bi_generate_prime(bi, bit_length);
		bi_setbit(bi, 0);
		bi_setbit(bi, bit_length - 1);
		bi_setbit(bi, bit_length - 2);
	}
}

/* This method generates small prime numbers up to a specified bounds using the Sieve of
 * Eratosthenes algorithm.
 *
 * prime_bound: the upper bound for the primes to be generated
 * starting_prime: the first prime in the list of primes that is returned
 * return: list of primes up to the specified bound. Each prime is of type bi_ptr
 */
void
generate_small_primes(int prime_bound, int starting_prime)
{
	list_ptr res;
	int length;
	int *is_primes;
	int i;
	int k;
	int prime;
	node_t *current;

	primes_length = 0;
	res = list_new();
	if (allocs == NULL) {
		LogError("malloc of list failed");
		return;
	}
	if ((prime_bound <= 1) || (starting_prime > prime_bound))
		return;

	if (starting_prime <= 2) {
		starting_prime = 2;
		list_add(res, bi_2);
	}
	length = (prime_bound - 1) >> 1; // length = (prime_bound -1) / 2;
	is_primes = (int *)malloc(sizeof(int)*length);
	if (is_primes == NULL) {
		LogError("malloc of %zd bytes failed", sizeof(int) * length);
		return;
	}

	for (i = 0; i < length; i++)
		is_primes[i] = 1;

	for (i = 0; i < length; i++) {
		if (is_primes[i] == 1) {
			prime = 2 * i + 3;
			for (k = i + prime; k < length; k+= prime)
				is_primes[k] = 0;

			if (prime >= starting_prime) {
				list_add(res, (void *)prime);
				primes_length++;
			}
		}
	}
	// converti the list to a table
	current = res->head; // go to first node
	primes  = (unsigned long *)malloc(sizeof(unsigned long) * primes_length);
	if (primes == NULL) {
		LogError("malloc of %d bytes failed",
			sizeof(unsigned long)*primes_length);
		return;
	}

	i = 0;
	while (current != NULL) {
		primes[i++] = (unsigned long)current->obj;
		current = current->next; // traverse through the list
	}

	free(is_primes);
	list_freeall(res);
}

void
prime_init()
{
	generate_small_primes(16384, 3);
}

/* Test whether the provided pDash or p = 2*pDash + 1 are divisible by any of the small primes
 * saved in the listOfSmallPrimes. A limit for the largest prime to be tested against can be
 * specified, but it will be ignored if it exeeds the number of precalculated primes.
 *
 * p_dash: the number to be tested (p_dash)
 * prime_bound: the limit for the small primes to be tested against.
 */
static int
test_small_prime_factors(const bi_ptr p_dash, const unsigned long prime_bound)
{
	int sievePassed = 1;
	unsigned long r;
	unsigned long small_prime;
	bi_t temp; bi_new(temp);

	small_prime = 1;
	int i = 0;
	while (i < primes_length && small_prime < prime_bound ) {
		small_prime = primes[i++];
		// r = p_dash % small_prime
		bi_mod_si(temp, p_dash, small_prime);
		r = bi_get_si(temp);
		// test if pDash = 0 (mod smallPrime)
		if (r == 0) {
			sievePassed = 0;
			break;
		}
		// test if p = 0 (mod smallPrime) (or r == smallPrime - r - 1)
		if (r == (small_prime - r - 1)) {
			sievePassed = 0;
			break;
		}
	}
	bi_free(temp);

	return sievePassed;
}

/* Tests if a is a Miller-Rabin witness for n
 *
 * a: number which is supposed to be the witness
 * n: number to be tested against
 * return: true if a is Miller-Rabin witness for n, false otherwise
 */
int
is_miller_rabin_witness(const bi_ptr a, const bi_ptr n)
{
	bi_t n_1;
	bi_t temp;
	bi_t _2_power_t;
	bi_t u;
	bi_t x0;
	bi_t x1;
	int t = -1;
	int i;

	bi_new(n_1);
	bi_new(temp);
	bi_new(_2_power_t);
	bi_new(u);

	// n1 = n - 1
	bi_sub_si(n_1, n, 1);

	// test if n-1 = 2^t*u with t >= 1 && u even
	do {
		t++;
		// _2_power_t = bi_1 << t  ( ==  2 ^ t)
		bi_shift_left(_2_power_t, bi_1, t);
		// u = n_1 / (2 ^ t)
		bi_div(u, n_1, _2_power_t);
	} while (bi_equals_si(bi_mod(temp, u, bi_2), 0));

	bi_new(x0);
	bi_new(x1);

	// x1 = (a ^ u ) % n
	bi_mod_exp(x1, a, u, n);

	// finished to use u, _2_power_t and temp
	bi_free(u);
	bi_free(_2_power_t);
	bi_free(temp);
	for (i = 0; i < t; i++) {
		bi_set(x0, x1);

		// x1 = (x0 ^ 2) % n
		bi_mod_exp(x1, x0, bi_2, n);
		if (bi_equals_si(x1, 1) && !bi_equals_si(x0, 1) && !bi_equals(x0, n_1) != 0) {
			bi_free(x0);
			bi_free(x1);
			bi_free(n_1);
			return 1;
		}
	}

	bi_free(x0);
	bi_free(x1);
	bi_free(n_1);

	if (!bi_equals(x1, bi_1))
		return 1;

	return 0;
}

bi_ptr
compute_trivial_safe_prime(bi_ptr result, int bit_length)
{
	LogDebugFn("Enter");
	do {
		bi_generate_prime(result, bit_length-1);
		bi_shift_left(result, result, 1);	// result := result << 1
		bi_add_si(result, result, 1);	// result := result -1
		if (getenv("TSS_DEBUG_OFF") == NULL) {
			printf(".");
			fflush(stdout);
		}
	} while (bi_is_probable_prime(result)==0);

	return result;
}

/* The main method to compute a random safe prime of the specified bit length.
 * IMPORTANT: The computer prime will have two first bits and the last bit set to 1 !!
 * i.e. > (2^(bitLength-1)+2^(bitLength-2)+1). This is done to be sure that if two primes of
 * bitLength n are multiplied, the result will have the bitLenght of 2*n exactly This
 * implementation uses the algorithm proposed by Ronald Cramer and Victor Shoup in "Signature
 * Schemes Based on the strong RSA Assumption" May 9, 2000.
 *
 * bitLength: the bit length of the safe prime to be computed.
 * return: a number which is considered to be safe prime
 */
bi_ptr
compute_safe_prime(bi_ptr p, int bit_length)
{
	bi_ptr p_dash;
	bi_ptr temp_p;
	bi_ptr p_minus_1;
	int stop;
	unsigned long prime_bound;

	LogDebug("compute Safe Prime: length: %d bits\n", bit_length);

	p_dash = bi_new_ptr();
	temp_p = bi_new_ptr();
	p_minus_1 = bi_new_ptr();

	/* some heuristic checks to limit the number of small primes to check against and the
	 * number of Miller-Rabin primality tests at the end */
	if (bit_length <= 256) {
		prime_bound = 768;
	} else if (bit_length <= 512) {
		prime_bound = 3072;
	} else if (bit_length <= 768) {
		prime_bound = 6144;
	} else if (bit_length <= 1024) {
		prime_bound = 1024;
	} else {
		prime_bound = 16384;
	}

	do {
		stop = 0;
		/* p_dash = generated random with basic bit settings (odd) */
		random_odd_bi(p_dash, bit_length - 1);

		if (test_small_prime_factors(p_dash, prime_bound) == 0)  {
			LogDebugFn("1");
			continue;
		}
		/* test if p_dash or p are divisible by some small primes */
		if (is_miller_rabin_witness(bi_2, p_dash)) {
			LogDebugFn("2");
			continue;
		}
		/* test if 2^(pDash) = +1/-1 (mod p)
		 * bi can not handle negative operation, we compare to (p-1) instead of -1
		 * calculate p = 2*pDash+1 -> (pDash << 1) + 1
		 */
		bi_shift_left(p, p_dash, 1);
		bi_add(p, p, bi_1);

		// p_minus_1:= p - 1
		bi_sub(p_minus_1, p, bi_1);

		//  temp_p := ( 2 ^ p_dash ) mod p
		bi_mod_exp(temp_p, bi_2, p_dash, p);
		if (!bi_equals_si(temp_p, 1)  && !bi_equals(temp_p, p_minus_1) ) {
			LogDebugFn("3");
			continue;
		}

		// test if pDash or p are divisible by some small primes
		if (is_miller_rabin_witness(bi_2, p_dash)) {
			LogDebugFn("4");
			continue;
		}
		// test the library dependent probable_prime
		if (bi_is_probable_prime(p_dash))
			stop = 1;
	} while (stop == 0);

	bi_free(p_minus_1);
	bi_free(temp_p);
	bi_free(p_dash);

	LogDebug("found Safe Prime: %s bits", bi_2_hex_char(p));

	return p;
}
