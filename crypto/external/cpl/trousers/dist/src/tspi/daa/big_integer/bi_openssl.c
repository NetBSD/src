
/*
 * Licensed Materials - Property of IBM
 *
 * trousers - An open source TCG Software Stack
 *
 * (C) Copyright International Business Machines Corp. 2006
 *
 */

#ifdef BI_OPENSSL

#define INSIDE_LIB

#include <time.h>
#include <stdio.h>
#include <stdlib.h>

#include <bi.h>

#include "tcslog.h"

#undef INLINE_DECL
#define INLINE_DECL

#include <string.h>
#include <openssl/rand.h>

BN_CTX *context;
static int initialized = 0;

void * (*bi_alloc)(size_t size);

/* return true if <i> is a probably prime  */
INLINE_DECL int bi_is_probable_prime( const bi_ptr i) {
	/*
	* start tests using some small prime numbers. Continue by performing a Miller-Rabin
	* probabilistic primality test with checks iterations, and with  some iterations that
	* yields a false positive rate of at most 2^-80 for random input.
	* return:
	*	0 if the number is composite
	*	1 if it is prime with an error probability of less than 0.25^checks, and on error.
	*/
	return BN_is_prime_fasttest( i, BN_prime_checks, NULL, context, NULL, 1);
}

/*  <result> := ( <g> ^ <e> ) mod <m>  */
INLINE_DECL bi_ptr bi_mod_exp_si( bi_ptr result, const bi_ptr g, const bi_ptr e, const long m) {
	bi_t bi_tmp;

	bi_new( bi_tmp);
	BN_set_word( bi_tmp, m);
#ifdef BI_DEBUG
	printf("[bi_mod_exp] (g=%s ^ e=%s) mod=%s\n",
		BN_bn2dec( g),
		BN_bn2dec( e),
		BN_bn2dec( bi_tmp));
#endif
	BN_mod_exp( result, g, e, bi_tmp, context);	// result := (g ^ e) mod bi_tmp9
#ifdef BI_DEBUG
	printf("[bi_mod_exp] res=%s\n", BN_bn2dec( result));
#endif
	bi_free( bi_tmp);
	return result;
}

/* multiple-exponentiation */
/* <result> := mod( Multi( <g>i, <e>i), number of byte <m>) with  0 <= i <= <n> */
bi_ptr bi_multi_mod_exp( bi_ptr result,
			const int n,
			const bi_t g[],
			const long e[],
			const int m
) {
	BIGNUM *temp = BN_new();
	BIGNUM *bi_m = BN_new();
	BIGNUM *bi_e = BN_new();
	int i;

	BN_set_word( bi_m, m);
	BN_set_word( bi_e, e[0]);
	// result := (g[0] ^ e[0]) mod bi_m
	BN_mod_exp( result, g[0], bi_e, bi_m, context);
	for( i=1; i<n; i++) {
		BN_set_word( bi_e, e[i]);
		// temp := (g[i] ^ e[i]) mod bi_m
		BN_mod_exp( temp, g[i], bi_e, bi_m, context);
		// result := result * temp
		BN_mul( result, result, temp, context);
	}
	BN_mod( result, result, bi_m, context);
	BN_free(bi_e);
	BN_free(bi_m);
	BN_free(temp);
	return result;
}


/***********************************************************************************
  					          INIT/RELEASE LIBRARY
************************************************************************************/

/* bi_alloc_p allocation function used only for exporting a bi struct,
so for bi_2_nbin? for example. if define as NULL, a stdlib malloc() will be used
*/
void bi_init( void * (*bi_alloc_p)(size_t size)) {
	if( initialized == 1) return;
	if( bi_alloc_p == NULL) bi_alloc = &malloc;
	else bi_alloc = bi_alloc_p;
	bi_new( bi_0);
	bi_set_as_si( bi_1, 0);
	bi_new( bi_1);
	bi_set_as_si( bi_1, 1);
	bi_new( bi_2);
	bi_set_as_si( bi_2, 2);
	allocs = list_new();
	if( allocs == NULL) {
		LogError("malloc of list failed");
		return;
	}
	LogDebug("bi_init() -> openssl lib\n");
	LogDebug("bi_init() -> seed status = %d\n", RAND_status());
	context = BN_CTX_new();
	if( RAND_status() != 1) {
		LogError("! PRNG has not been seeded with enough data\n");
#ifdef INTERACTIVE
		printf("type some characters to regenerate a random seed\n");
		fflush(stdout);
		int c, i=0, seed;
		char str[2] = "a";
		while( RAND_status() !=1 ) {
			c = fgetc(stdin);
			time_t temps;
			time( &temps);
			seed += temps +( c << i);
			i++;
			str[0]=c;
			RAND_seed( str, 1);
		}
#endif
	}
	initialized = 1;
}

void bi_release(void) {
	if( initialized) {
		bi_flush_memory();
		bi_free( bi_0);
		bi_free( bi_1);
		bi_free( bi_2);
		BN_CTX_free( context);
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
			LogDebug("[flush memory] free %lx\n", (long)current->obj);
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
