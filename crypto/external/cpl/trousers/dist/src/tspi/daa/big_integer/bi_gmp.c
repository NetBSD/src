
/*
 * Licensed Materials - Property of IBM
 *
 * trousers - An open source TCG Software Stack
 *
 * (C) Copyright International Business Machines Corp. 2006
 *
 */

#ifdef BI_GMP

#include <time.h>
#include <stdio.h>
#include <stdlib.h>

#include <bi.h>

#undef INLINE_DECL
#define INLINE_DECL

gmp_randstate_t state;

/*
 reps controls how many  tests should be done in mpz_probable_prime_p:
 5 to 10 are reasonable number
*/
static int reps = 20;

static int initialized = 0;
void * (*bi_alloc)(size_t size);

/***********************************************************************************
	BITS OPERATION
*************************************************************************************/

// for conversion from and to byte[] see mpz_export and mpz_import

/* return the size of a network byte order representation of <i>  */
long bi_nbin_size(const bi_ptr i) {
	int word_size = 1; // 1 byte per word
	int numb = 8 * word_size - 0;
	int count = (mpz_sizeinbase ( i, 2) + numb-1) / numb;

	return count * word_size;
}

/* return a BYTE * - in network byte order -  and update the length <length> */
unsigned char *bi_2_nbin( int *length, const bi_ptr i) {
	unsigned char *buffer = (unsigned char *)bi_alloc( bi_nbin_size( i));

	if( buffer == NULL) return NULL;
	mpz_export(buffer, length, 1, 1, 1, 0, i);
	return buffer;
}

/* return a BYTE * - in network byte order -  and update the length <length>  */
/* different from bi_2_nbin: you should reserve enough memory for the storage */
void bi_2_nbin1( int *length, unsigned char *buffer, const bi_ptr i) {
	mpz_export(buffer, length, 1, 1, 1, 0, i);
}

/* return a bi_ptr that correspond to the big endian encoded BYTE array of length <n_length> */
INLINE_DECL bi_ptr bi_set_as_nbin( const unsigned long length, const unsigned char *buffer) {
	bi_ptr ret = bi_new_ptr();

	if( ret == NULL) return NULL;
	mpz_import( ret, length, 1, 1, 1, 0, buffer);
	return ret;
}


/***********************************************************************************
	BASIC MATH OPERATION
*************************************************************************************/

/* multiple-exponentiation */
/* <result> := mod( Multi( <g>i, <e>i), number of byte <m>) with  0 <= i <= <n> */
bi_ptr bi_multi_mod_exp( bi_ptr result,
			const int n,
			const bi_t g[],
			const long e[],
			const int m) {
	mpz_t temp, bi_m;
	int i;

	mpz_init( temp);
	mpz_init( bi_m);
	mpz_set_si( bi_m, m);
	// result := (g[0] ^ e[0]) mod m
	mpz_powm_ui( result, g[0], e[0], bi_m);
	for( i=1; i<n; i++) {
		//  temp := (g[i] ^ e[i]) mod bi_m
		mpz_powm_ui( temp, g[i], e[i], bi_m);
		//  result := result * temp
		mpz_mul( result, result, temp);
	}
	mpz_mod_ui( result, result, m);
	mpz_clear( bi_m);
	mpz_clear( temp);
	return result;
}

/***********************************************************************************
	NUMBER THEORIE OPERATION
*************************************************************************************/
/* generate prime number of <length> bits  */
INLINE_DECL bi_ptr bi_generate_prime( bi_ptr result, const long length) {
	do {
		mpz_urandomb( result, state, length);	// i := random( length)
		bi_setbit( result, 0);
		bi_setbit( result, length - 1);
		bi_setbit( result, length - 2);
		mpz_nextprime( result, result);	// i := nextPrime( i)
	} while( mpz_sizeinbase( result, 2) != (unsigned long)length &&
			bi_is_probable_prime( result) );
	return result;
}

/* generate a safe prime number of <length> bits  */
/* by safe we mean a prime p so that (p-1)/2 is also prime */
INLINE_DECL bi_ptr bi_generate_safe_prime( bi_ptr result, long length) {
	mpz_t temp;

	mpz_init(temp);
	do {
		bi_generate_prime( result, length);
		mpz_sub_ui( temp, result, 1); // temp := result - 1
		mpz_div_2exp( temp, temp, 1); // temp := temp / 2
	} while( mpz_probab_prime_p( temp, 10) == 0);
#ifdef BI_DEBUG
	printf("GENERATE SAFE PRIME DONE");fflush(stdout);
#endif
	mpz_clear( temp);
	return result;
}

/* return true if <i> is a probably prime  */
INLINE_DECL int bi_is_probable_prime( bi_ptr i) {
	/*
	This function does some trial divisions and after some Miller-Rabin probabilistic
	primality tests. The second parameter controls how many  tests should be done,
	5 to 10 are reasonable number
	*/
	return mpz_probab_prime_p( i, reps)>=1 ? 1 : 0;
}

/* return in <result> the greatest common divisor of <a> and <b> */
/* <result> := gcd( <a>, <b>) */
INLINE_DECL bi_ptr bi_gcd( bi_ptr result, bi_ptr a, bi_ptr b) {
	// result := gcd( a, b)
	mpz_gcd( result, a, b);
	return result;
}


/***********************************************************************************
	INIT/RELEASE LIBRARY
*************************************************************************************/

/* bi_alloc_p allocation function used only for exporting a bi struct, so for bi_2_nbin
if define as NULL, a stdlib malloc() will be used */
void bi_init( void * (*bi_alloc_p)(size_t size)) {
	time_t start;
	unsigned long seed;
	FILE *f;
#ifdef INTERACTIVE
	int c, i;
#endif

	if( initialized == 1) return;
	if( bi_alloc_p == NULL) bi_alloc = &malloc;
	else bi_alloc = bi_alloc_p;
	mpz_init( bi_0);
	mpz_set_ui( bi_0, 0);
	mpz_init( bi_1);
	mpz_set_ui( bi_1, 1);
	mpz_init( bi_2);
	mpz_set_ui( bi_2, 2);
	allocs = list_new();
	if( allocs == NULL) {
		LogError("malloc of list failed");
		return;
	}
#ifdef BI_DEBUG
	printf("bi_init() -> gmp lib\n");
#endif
	time( &start);
	// first try /dev/random, the most secure random generator
	f = fopen("/dev/random", "r");
	// in case of failure, but les secure :(
	if( f== NULL) f = fopen("/dev/urandom", "r");
	if( f != NULL) {
		fread( &seed, sizeof(unsigned long int), 1, f);
		fclose(f);
	}
#ifdef INTERACTIVE
	else {
		printf("! devices /dev/random and /dev/urandom not found\n");
		printf("type some characters to generate a random seed (follow by enter)\n");
		fflush(stdout);
		i=0;
		while( (c = fgetc(stdin)) != 10) {
			time_t temps;
			time( &temps);
			seed += temps +( c << i);
			i++;
		}
		time_t temps;
		time( &temps);
		seed += temps;
#ifdef BI_DEBUG
		printf("temps=%lx\n", temps - start);
#endif // BI_DEBUG
		seed = (long)( temps * start);
	}
#endif // INTERACTIVE
#ifdef BI_DEBUG
	printf("seed=%lx\n", seed);
#endif
	gmp_randinit_default( state);
	gmp_randseed_ui( state, seed);
	initialized = 1;
}

void bi_release(void) {
	if( initialized) {
		bi_flush_memory();
		bi_free( bi_0);
		bi_free( bi_1);
		bi_free( bi_2);
		initialized = 0;
	}
}
void bi_flush_memory(void) {
	node_t *current;
	list_ptr list = allocs;

	if( list->head == NULL) return; // list is empty
	else {
		current = list->head; // go to first node
		do {
			LogDebug("[flush memory] free %lx\n", current->obj);
			free( current->obj);
			current = current->next; // traverse through the list
		} while(current != NULL); // until current node is NULL
	}
	list_freeall( allocs);
	allocs = list_new();
	if( allocs == NULL) {
		LogError("malloc of list failed");
		return;
	}
}


int bi_is_initialized(void) {
	return initialized;
}

#endif
