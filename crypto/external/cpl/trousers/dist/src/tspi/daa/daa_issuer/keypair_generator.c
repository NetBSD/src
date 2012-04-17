/*
 * Licensed Materials - Property of IBM
 *
 * trousers - An open source TCG Software Stack
 *
 * (C) Copyright International Business Machines Corp. 2006
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "bi.h"
#include "list.h"
#include "daa_structs.h"
#include "daa_parameter.h"
#include "issuer.h"

static const int ELEMENT = 0;
static const int EXPONENT = 1;

extern void prime_init();
extern void compute_safe_prime(bi_ptr result, int bit_length, int prime_certainty);

bi_ptr
compute_random_number_star( bi_ptr result, const bi_ptr element)
{
	bi_t bi_tmp;

	bi_new(bi_tmp);
	do {
		compute_random_number(result, element);
	} while (!bi_equals_si(bi_gcd(bi_tmp, result, element), 1));

	bi_free(bi_tmp);

	return result;
}

/* Compute a generator of the group of quadratic residue modulo n. The
 *  generator will not be part of the subgroup of size 2.
 * n: modulus */
void
compute_generator_quadratic_residue(bi_t qr, bi_t n)
{
	bi_t bi_tmp, bi_tmp1;

	bi_new(bi_tmp);
	bi_new(bi_tmp1);

	do {
		compute_random_number(qr, n);
		// qr = (qr ^ bi_2) % n
		bi_mod_exp(qr, qr, bi_2, n);
	} while (bi_cmp_si(qr, 1) == 0 ||
		bi_cmp_si(bi_gcd(bi_tmp, n, bi_sub_si(bi_tmp1, qr, 1)), 1) != 0);

	bi_free(bi_tmp);
	bi_free(bi_tmp1);
}

void
compute_group_element(bi_ptr result[],
		      bi_ptr generator,
		      bi_ptr product_PQprime,
		      bi_ptr n)
{
	bi_t bi_tmp;

	bi_new(bi_tmp);
	compute_random_number(bi_tmp, product_PQprime);

	// bi_tmp++
	bi_inc(bi_tmp);

	// result[ELEMENT] := (generator ^ bi_tmp) mod n
	bi_mod_exp(result[ELEMENT], generator, bi_tmp, n);
	bi_set(result[EXPONENT], bi_tmp);
	bi_free(bi_tmp);
}


TSS_RESULT
generate_key_pair(UINT32                         num_attributes_issuer,
		  UINT32                         num_attributes_receiver,
		  UINT32                         base_nameLength,
		  BYTE*                          base_name,
		  KEY_PAIR_WITH_PROOF_internal** key_pair_with_proof)
{
	TSS_RESULT result = TSS_SUCCESS;
	int length_mod = DAA_PARAM_SIZE_RSA_MODULUS;
	int length;
	int i;
	TSS_DAA_PK_internal *public_key = NULL;
	BYTE *buffer = NULL;
	bi_ptr pPrime = NULL;
	bi_ptr qPrime = NULL;
	bi_ptr n = NULL;
	bi_ptr p = NULL;
	bi_ptr q = NULL;
	bi_ptr capital_s = NULL;
	bi_ptr capital_z = NULL;
	bi_ptr product_PQprime = NULL;
	bi_ptr pair[2] = {NULL, NULL};
	bi_ptr xz = NULL;
	bi_ptr capital_r0 = NULL;
	bi_ptr x0 = NULL;
	bi_ptr capital_r1 = NULL;
	bi_ptr x1 = NULL;
	bi_array_ptr x = NULL;
	bi_array_ptr capital_r = NULL;
	bi_array_ptr capitalRReceiver = NULL;
	bi_array_ptr capitalRIssuer = NULL;
	bi_ptr gamma = NULL;
	bi_ptr capital_gamma = NULL;
	bi_ptr rho = NULL;
	bi_ptr r = NULL;
	bi_ptr rho_double = NULL;
	bi_t bi_tmp, bi_tmp1, bi_tmp2;

	bi_new(bi_tmp);
	bi_new(bi_tmp1);
	bi_new(bi_tmp2);
	*key_pair_with_proof = NULL;

	// STEP 1
	LogDebug("Step 1 of 8 - compute modulus n (please wait: long process)\n");

	// FUTURE USAGE if( IS_DEBUG==0)
	prime_init();
	p = bi_new_ptr();
	q = bi_new_ptr();
	n = bi_new_ptr();

	do {
		// FUTURE USAGE
		/* compute_safe_prime( p, length_mod / 2);
		do {
			compute_safe_prime( q,
							length_mod - (length_mod >> 1));
		} while( bi_cmp( p, q) ==0);
		} else */
		{
			bi_generate_safe_prime(p, length_mod / 2);
			bi_generate_safe_prime(q, length_mod - (length_mod / 2));
			LogDebug(".");
		}
		// n = p*q
		bi_mul(n, p, q);
	} while(bi_length(n) != length_mod);

	pPrime = bi_new_ptr();
	bi_sub(pPrime, p, bi_1);

	// pPrime = (p - 1) >> 1
	bi_shift_right(pPrime, pPrime, 1);
	qPrime = bi_new_ptr();
	bi_sub(qPrime, q, bi_1);

	// qPrime = (q - 1) >> 1
	bi_shift_right( qPrime, qPrime, 1);
	if (bi_is_probable_prime(pPrime) == 0) {
		LogError("!! pPrime not a prime number: %s", bi_2_hex_char(pPrime));
		result = TSPERR(TSS_E_INTERNAL_ERROR);
		goto close;
	}
	if (bi_is_probable_prime(qPrime) == 0) {
		LogError("!! qPrime not a prime number: %s", bi_2_hex_char(qPrime));
		result = TSPERR(TSS_E_INTERNAL_ERROR);
		goto close;
	}
	LogDebug("p=%s", bi_2_hex_char(p));
	LogDebug("q=%s", bi_2_hex_char(q));
	LogDebug("n=%s", bi_2_hex_char(n));

	// STEP 2
	LogDebug("Step 2 - choose random generator of QR_n");
	capital_s = bi_new_ptr();
	compute_generator_quadratic_residue(capital_s, n);
	LogDebug("capital_s=%s", bi_2_hex_char(capital_s));

	// STEP 3 & 4
	LogDebug("Step 3 & 4 - compute group elements");
	product_PQprime = bi_new_ptr();
	bi_mul( product_PQprime, pPrime, qPrime);
	pair[ELEMENT] = bi_new_ptr();
	pair[EXPONENT] = bi_new_ptr();

	LogDebug("product_PQprime=%s [%ld]", bi_2_hex_char(product_PQprime),
		 bi_nbin_size(product_PQprime));

	compute_group_element(pair, capital_s, product_PQprime, n);
	capital_z = bi_new_ptr();
	bi_set(capital_z, pair[ELEMENT]);
	xz = bi_new_ptr();
	bi_set(xz,  pair[EXPONENT]);

	// attributes bases
	compute_group_element(pair, capital_s, product_PQprime, n);
	capital_r0 = bi_new_ptr();
	bi_set(capital_r0, pair[ELEMENT]);
	x0 = bi_new_ptr();
	bi_set(x0, pair[EXPONENT]);

	compute_group_element(pair, capital_s, product_PQprime, n);
	capital_r1 = bi_new_ptr();
	bi_set(capital_r1, pair[ELEMENT]);
	x1 = bi_new_ptr();
	bi_set(x1, pair[EXPONENT]);

	// additional attribute bases
	length = num_attributes_issuer + num_attributes_receiver;
	x = ALLOC_BI_ARRAY();
	bi_new_array(x, length);
	capital_r = ALLOC_BI_ARRAY();
	bi_new_array(capital_r, length);

	for (i = 0; i < length; i++) {
		compute_group_element(pair, capital_s, product_PQprime, n);
		bi_set(capital_r->array[i], pair[ELEMENT]);
		bi_set(x->array[i], pair[EXPONENT]);
	}

	// split capitalR into Receiver and Issuer part
	capitalRReceiver = ALLOC_BI_ARRAY();
	bi_new_array2(capitalRReceiver, num_attributes_receiver);
	for (i = 0; i < num_attributes_receiver; i++)
		capitalRReceiver->array[i] = capital_r->array[i];
	capitalRIssuer = ALLOC_BI_ARRAY();
	bi_new_array2(capitalRIssuer, num_attributes_issuer);
	for (i = 0; i < num_attributes_issuer; i++)
		capitalRIssuer->array[i] = capital_r->array[i + num_attributes_receiver];

	// STEP 6a
	LogDebug("Step 6");
	gamma = bi_new_ptr();
	capital_gamma = bi_new_ptr();
	rho = bi_new_ptr();
	r = bi_new_ptr();
	rho_double = bi_new_ptr();

	bi_generate_prime(rho, DAA_PARAM_SIZE_RHO);
	if (bi_length(rho) != DAA_PARAM_SIZE_RHO) {
		LogError("rho bit length=%ld", bi_length(rho));
		result = TSPERR(TSS_E_INTERNAL_ERROR);
		goto close;
	}

	do {
		length = DAA_PARAM_SIZE_MODULUS_GAMMA - DAA_PARAM_SIZE_RHO;
		do {
			bi_urandom(r, length);
		} while(bi_length(r) != length || bi_equals_si(bi_mod(bi_tmp, r, rho), 0));

		// rho is not a dividor of r
		bi_mul( capital_gamma, rho, r);
		// capital_gamma ++
		bi_inc( capital_gamma);
#ifdef DAA_DEBUG
		if (bi_length(capital_gamma) != DAA_PARAM_SIZE_MODULUS_GAMMA) {
			printf("|"); fflush(stdout);
		} else {
			printf("."); fflush(stdout);
		}
#endif
	} while (bi_length(capital_gamma) != DAA_PARAM_SIZE_MODULUS_GAMMA ||
		 bi_is_probable_prime(capital_gamma) == 0 );

	// STEP 6b
	if (bi_equals(bi_sub_si(bi_tmp, capital_gamma, 1),
			bi_mod(bi_tmp1, bi_mul(bi_tmp2, rho, r), n)) == 0) {
		LogWarn("capital_gamma-1 != (rho * r) mod n  tmp=%s  tmp1=%s",
			bi_2_hex_char(bi_tmp), bi_2_hex_char(bi_tmp1));
	}

	if (bi_equals(bi_div(bi_tmp, bi_sub_si(bi_tmp1, capital_gamma, 1), rho), r ) == 0) {
		LogWarn("( capital_gamma - 1)/rho != r");
	}

	LogDebug("capital_gamma=%s\n", bi_2_hex_char(capital_gamma));
	do {
		compute_random_number_star(gamma, capital_gamma);
		// gamma = (gamma ^ r) mod capital_gamma
		bi_mod_exp(gamma, gamma, r, capital_gamma);
	} while (bi_equals(gamma, bi_1));
	// STEP 7
	buffer = (BYTE *)malloc(base_nameLength);
	if (buffer == NULL) {
		LogError("malloc of %u bytes failed", base_nameLength);
		result = TSPERR(TSS_E_OUTOFMEMORY);
		goto close;
	}
	memcpy(buffer, base_name, base_nameLength);
	// all fields are linked to the struct with direct reference
	public_key = create_DAA_PK(n, capital_s, capital_z, capital_r0, capital_r1, gamma,
				   capital_gamma, rho, capitalRReceiver, capitalRIssuer,
				   base_nameLength, buffer);

	// STEP 8
	// TODO dynamically load DAAKeyCorrectnessProof
	LogDebug("Step 8: generate proof (please wait: long process)");
	TSS_DAA_PK_PROOF_internal *correctness_proof = generate_proof(product_PQprime, public_key,
								      xz, x0, x1, x);
	if (correctness_proof == NULL) {
		LogError("creation of correctness_proof failed");
		result = TSPERR(TSS_E_OUTOFMEMORY);
		goto close;
	}

	*key_pair_with_proof = (KEY_PAIR_WITH_PROOF_internal *)
					malloc(sizeof(KEY_PAIR_WITH_PROOF_internal));
	if (*key_pair_with_proof == NULL) {
		LogError("malloc of %zd bytes failed", sizeof(KEY_PAIR_WITH_PROOF_internal));
		result = TSPERR(TSS_E_OUTOFMEMORY);
		goto close;
	}

	(*key_pair_with_proof)->pk = public_key;
	(*key_pair_with_proof)->proof = correctness_proof;
	// all fields are linked to the struct with direct reference
	(*key_pair_with_proof)->private_key = create_TSS_DAA_PRIVATE_KEY(pPrime, qPrime);
close:
	if (result != TSS_SUCCESS) {
		// remove everything, even numbers that should be stored in a struct
		FREE_BI(pPrime);	// kept if no error
		FREE_BI(qPrime);	// kept if no error
		FREE_BI(n);	// kept if no error
		// FREE_BI( p);
		// FREE_BI( q);
		FREE_BI(capital_s);	// kept if no error
		FREE_BI(capital_z);	// kept if no error
		// FREE_BI(product_PQprime);
		// FREE_BI(pair[ELEMENT]);
		// FREE_BI(pair[EXPONENT]);
		// FREE_BI(xz);
		FREE_BI(capital_r0);	// kept if no error
		// FREE_BI(x0);
		FREE_BI(capital_r1);	// kept if no error
		// FREE_BI( x1);
		// bi_array_ptr x = NULL;
		// bi_array_ptr capital_r = NULL;
		// bi_array_ptr capitalRReceiver = NULL;
		// bi_array_ptr capitalRIssuer = NULL;
		FREE_BI( gamma);	// kept if no error
		FREE_BI( capital_gamma);	// kept if no error
		FREE_BI( rho);	// kept if no error
		// FREE_BI( r);
		// FREE_BI( rho_double);
		if (buffer!=NULL)
			free(buffer);

		if (public_key != NULL)
			free(public_key);

		if (*key_pair_with_proof != NULL)
			free(*key_pair_with_proof);
	}
	/*
	Fields kept by structures
	TSS_DAA_PK: 	n
				capital_s
				capital_z
				capital_r0
				capital_r1
				gamma
				capital_gamma
				rho
				capitalRReceiver
				capitalRIssuer
				base_nameLength
				buffer
	TSS_DAA_PRIVATE_KEY:
				pPrime
				qPrime
	*/
	bi_free(bi_tmp);
	bi_free(bi_tmp1);
	bi_free(bi_tmp2);
	FREE_BI(p);
	FREE_BI(q);
	FREE_BI(product_PQprime);
	FREE_BI(pair[ELEMENT]);
	FREE_BI(pair[EXPONENT]);
	FREE_BI(xz);
	FREE_BI(x0);
	FREE_BI(x0);
	// bi_array_ptr x = NULL;
	// bi_array_ptr capital_r = NULL;
	// bi_array_ptr capitalRReceiver = NULL;
	// bi_array_ptr capitalRIssuer = NULL;
	FREE_BI(r);
	FREE_BI(rho_double);

	return result;
}
