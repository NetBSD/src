
/*
 * Licensed Materials - Property of IBM
 *
 * trousers - An open source TCG Software Stack
 *
 * (C) Copyright International Business Machines Corp. 2006
 *
 */

#ifndef BI_GMP_
#define BI_GMP_

#include <gmp.h>
// needed for OPENSSL_free
#include <openssl/crypto.h>

typedef mpz_t bi_t;
typedef mpz_ptr bi_ptr;

extern gmp_randstate_t state;

/* 	initialized the given big integer */
INLINE_DECL bi_ptr bi_new(bi_t i) {
	mpz_init( i);
	return i;
}

/* create a big integer pointer */
INLINE_DECL bi_ptr bi_new_ptr(void) {
	mpz_ptr res;

	res = (mpz_ptr)malloc( sizeof( mpz_t));
	if( res == NULL) return NULL;
	mpz_init( res);
	return res;
}

/* free resources allocated to the big integer <i> */
INLINE_DECL void bi_free(const bi_ptr i) {
	mpz_clear( i);
}

/* free resources allocated to the big integer pointer <i> */
INLINE_DECL void bi_free_ptr(const bi_ptr i) {
	mpz_clear( i);
	free( i);
}

/* return the current number of bits of the number */
INLINE_DECL long bi_length( const bi_ptr res) {
	return mpz_sizeinbase( res, 2);
}

/***********************************************************************************
	CONVERSIONS
*************************************************************************************/


/* return an hex dump of the given big integer  */
INLINE_DECL char *bi_2_hex_char(const bi_ptr i) {
	char *ret;

	gmp_asprintf( &ret, "%ZX", i);
	list_add( allocs, ret);
	return ret;
}

/* return an hex dump of the given big integer  */
INLINE_DECL char *bi_2_dec_char(const bi_ptr i) {
	char *ret;

	gmp_asprintf( &ret, "%Zd", i);
	list_add( allocs, ret);
	return ret;
}

/* set <i> to the same value as the big integer <value> */
INLINE_DECL bi_ptr bi_set( bi_ptr result, const bi_ptr value) {
	mpz_set( result, value);
	return result;
}

/* set the initialized variable to the value represented by the given hex format stirng */
INLINE_DECL bi_ptr bi_set_as_hex( bi_ptr result, const char *value) {
	mpz_set_str( result, value, 16);
	return result;
}

/* set the initialized variable to the value represented by the given hex format stirng */
INLINE_DECL bi_ptr bi_set_as_dec( bi_ptr result, const char *value) {
	mpz_set_str( result, value, 10);
	return result;
}

/* set <i> with the value represented by unsigned int <value> */
/*    <i> := <value>          */
INLINE_DECL bi_ptr bi_set_as_si( bi_ptr result, const int value) {
	mpz_set_si( result, value);
	return result;
}

/* return (long)bi_t  */
INLINE_DECL long bi_get_si(const bi_ptr i) {
	return mpz_get_si( i);
}

/* convert a bi type to a openssl BIGNUM struct */
INLINE_DECL BIGNUM *bi_2_BIGNUM( const bi_ptr i) {
	BIGNUM *result;
	char *value = bi_2_hex_char( i);

	BN_hex2bn( &result, value);
	return result;
}

/* set <i> with the value represented by the given openssl BIGNUM struct */
INLINE_DECL bi_ptr bi_set_as_BIGNUM( bi_ptr i, BIGNUM *bn) {
	char *value = BN_bn2hex( bn);

	if( value == NULL) return NULL;
	bi_set_as_hex( i, value);
	OPENSSL_free( value);
	return i;
}

/***********************************************************************************
	BASIC MATH OPERATION
*************************************************************************************/

/* <result> := result + 1 */
INLINE_DECL bi_ptr bi_inc(bi_ptr result) {
	mpz_add_ui( result, result, 1);
	return result;
}

/* <result> := result - 1 */
INLINE_DECL bi_ptr bi_dec(bi_ptr result) {
	mpz_sub_ui( result, result, 1);
	return result;
}

/* set <result> by the division of <i> by the long <n>  */
/*  <result> := <i> / <n>   */
INLINE_DECL bi_ptr bi_div_si( bi_ptr result, const bi_ptr i, const long n) {
	mpz_div_ui( result, i, n);
	return result;
}

/*  <result> := <i> / <n>   */
INLINE_DECL bi_ptr bi_div( bi_ptr result, const bi_ptr i, const bi_ptr n) {
	mpz_div( result, i, n);
	return result;
}

/* <result> := - <result> */
INLINE_DECL bi_ptr bi_negate( bi_ptr result) {
	mpz_neg( result, result);
	return result;
}

/* multiply the given big integer <i> by the give long <n> and return the result in <result> */
INLINE_DECL bi_ptr bi_mul_si( bi_ptr result, const bi_ptr i, const long n) {
	mpz_mul_si( result,  i, n);
	return result;
}

 /*  <result> := <i> * <n>   */
INLINE_DECL bi_ptr bi_mul( bi_ptr result, const bi_ptr i, const bi_ptr n) {
	mpz_mul( result, i, n);
	return result;
}

/*  <result> := <i> + <n>  */
INLINE_DECL bi_ptr bi_add_si( bi_ptr result, const bi_ptr i, const long n) {
	mpz_add_ui( result, i, n);
	return result;
}

/*  <result> := <i> + <n>  */
INLINE_DECL bi_ptr bi_add( bi_ptr result, const bi_ptr i, const bi_ptr n) {
	mpz_add( result, i, n);
	return result;
}

/*  <result> := <i> - <n>   */
INLINE_DECL bi_ptr bi_sub_si( bi_ptr result, const bi_ptr i, const long n) {
	// n should be unsigned
	mpz_sub_ui( result, i, n);
	return result;
}

/*  <result> := <i> - <n>  */
INLINE_DECL bi_ptr bi_sub( bi_ptr result, const bi_ptr i, const bi_ptr n) {
	mpz_sub( result, result, n);
	return result;
}

/*  <result> := ( <g> ^ <e> ) mod <m>  */
INLINE_DECL bi_ptr bi_mod_exp( bi_ptr result, const bi_ptr g, const bi_ptr e, const bi_ptr m) {
	mpz_powm( result, g, e, m);
	return result;
}

/*  <result> := ( <g> ^ <e> ) mod <m>  */
INLINE_DECL bi_ptr bi_mod_exp_si( bi_ptr result, const bi_ptr g, const bi_ptr e, const long m) {
	mpz_t bi_m;

	mpz_init( bi_m);
	mpz_set_si( bi_m, m);
	mpz_powm( result, g, e, bi_m);
	mpz_clear( bi_m);
	return result;
}

/***********************************************************************************
	BITS OPERATION
*************************************************************************************/
/* set the bit to 1 */
INLINE_DECL bi_ptr bi_setbit(bi_ptr result, const int bit) {
	mpz_setbit( result, bit);
	return result;
}

/* <result> := <i> << <n> */
INLINE_DECL bi_ptr bi_shift_left( bi_ptr result, const bi_ptr i, const int n) {
	mpz_mul_2exp( result, i, n);
	return result;
}

/* <result> := <i> >> <n> */
INLINE_DECL bi_ptr bi_shift_right( bi_ptr result, const bi_ptr i, const int n) {
	mpz_div_2exp( result, i, n);
	return result;
}

/***********************************************************************************
	COMPARAISON
*************************************************************************************/
/*  n1<n2   return negative value
 *  n1 = n2 return 0
 *  n1>n2   return positive value
*/
INLINE_DECL int bi_cmp( const bi_ptr n1, const bi_ptr n2) {
	return mpz_cmp( n1, n2);
}

/*  n1<n2   return negative value
 *  n1 = n2 return 0
 *  n1>n2   return positive value
*/
INLINE_DECL int bi_cmp_si( const bi_ptr n1, const int n2) {
	return mpz_cmp_ui( n1, n2);
}

/*  n1 == n2   return 1 (true)
 *  else return 0
*/
INLINE_DECL int bi_equals( const bi_ptr n1, const bi_ptr n2) {
	return mpz_cmp( n1, n2) == 0 ? 1 : 0;
}

/*  n1 == n2   return 1 (true)
 *  else return 0
*/
INLINE_DECL int bi_equals_si( const bi_ptr n1, const int n2) {
	return mpz_cmp_ui( n1, n2) == 0 ? 1 : 0;
}

/* create a random of length <length> bits */
/*  res := random( length)  */
INLINE_DECL bi_ptr bi_urandom( bi_ptr result, const long length) {
	mpz_urandomb( result, state, length);
	return result;
}

/* res := <n> mod <m> */
INLINE_DECL bi_ptr bi_mod_si( bi_ptr result, const bi_ptr n, const long m) {
	mpz_mod_ui( result, n, m);
	return result;
}

/* res := <n> mod <m> */
INLINE_DECL bi_ptr bi_mod( bi_ptr result, const bi_ptr n, const bi_ptr m) {
	mpz_mod( result, n, m);
	return result;
}

/* result := (inverse of <i>) mod <m> */
/* if the inverse exist, return >0, otherwise 0 */
INLINE_DECL int bi_invert_mod( bi_ptr result, const bi_ptr i, const bi_ptr m) {
	return mpz_invert( result, i, m);
}

#endif /*BI_GMP_*/
