
/*
 * Licensed Materials - Property of IBM
 *
 * trousers - An open source TCG Software Stack
 *
 * (C) Copyright International Business Machines Corp. 2006
 *
 */

#ifndef BI_OPENSSL_
#define BI_OPENSSL_

#include <openssl/bn.h>
#include <openssl/engine.h>

typedef struct bignum_st bi_t[1];

typedef struct bignum_st *bi_ptr;

extern BN_CTX *context;


INLINE_DECL bi_ptr bi_new(bi_ptr result) {
	BN_init( result);
	return result;
}

/* create a big integer pointer */
INLINE_DECL bi_ptr bi_new_ptr(void) {
	return BN_new();
}

/* free resources allocated to the big integer <i> */
INLINE_DECL void bi_free(const bi_ptr i) {
	BN_free( i);
}

/* free resources allocated to the big integer pointer <i> */
INLINE_DECL void bi_free_ptr(const bi_ptr i) {
	BN_free( i);
}

/* <result> := result++ */
INLINE_DECL bi_ptr bi_inc(bi_ptr result) {
	BN_add_word( result, 1);
	return result;
}

/* <result> := result-- */
INLINE_DECL bi_ptr bi_dec(bi_ptr result) {
	BN_sub_word( result, 1);
	return result;
}

/* return the current number of bits of the number */
INLINE_DECL long bi_length( const bi_ptr res) {
	return BN_num_bits( res);
}

/***********************************************************************************
	BASIC MATH OPERATION
*************************************************************************************/
/* <result> := - <result> */
INLINE_DECL bi_ptr bi_negate( bi_ptr result) {
	BIGNUM *n = result;
	n->neg = ( n->neg == 0 ? 1 : 0);
	return result;
}

INLINE_DECL bi_ptr bi_mul_si( bi_ptr result, const bi_ptr i, const long n) {
	BN_copy( result, i);
	BN_mul_word( result, n);
	return result;
}

/*  <result> := <i> * <n>   */
INLINE_DECL bi_ptr bi_mul( bi_ptr result, const bi_ptr i, const bi_ptr n) {
	BN_mul( result, i, n, context);
	return result;
}

INLINE_DECL bi_ptr bi_add_si( bi_ptr result, const bi_ptr i, const long n) {
	BN_copy( result, i);
	BN_add_word( result, n);
	return result;
}

/*  <result> := <i> + <n>  */
INLINE_DECL bi_ptr bi_add( bi_ptr result, const bi_ptr i, const bi_ptr n) {
	BN_add( result, i, n);
	return result;
}

/*  <result> := <i> - <n>   */
INLINE_DECL bi_ptr bi_sub_si( bi_ptr result, const bi_ptr i, const long n) {
	// n should be unsigned
	BN_copy( result, i);  			  // result := i
	BN_sub_word( result, n);   // result := result - n
	return result;
}

/*  <result> := <i> - <n>  */
INLINE_DECL bi_ptr bi_sub( bi_ptr result, const bi_ptr i, const bi_ptr n) {
	BN_sub( result, i, n);
	return result;
}

/*  <result> := ( <g> ^ <e> ) mod <m>  */
INLINE_DECL bi_ptr bi_mod_exp( bi_ptr result, const bi_ptr g, const bi_ptr e, const bi_ptr m) {
	BN_mod_exp( result, g, e, m, context);	// result := (g ^ e) mod bi_m
	return result;
}

/* set <result> by the division of <i> by the long <n>  */
/*  <result> := <i> / <n>   */
INLINE_DECL bi_ptr bi_div_si( bi_ptr result, const bi_ptr i, const long n) {
	BN_copy( result, i);
	BN_div_word( result, n);
	return result;
}

/*  <result> := <i> / <n>   */
INLINE_DECL bi_ptr bi_div( bi_ptr result, const bi_ptr i, const bi_ptr n) {
	BN_div( result, NULL, i, n, context);
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
	return BN_cmp( n1, n2);
}

/*  n1<n2   return negative value
 *  n1 = n2 return 0
 *  n1>n2   return positive value
*/
INLINE_DECL int bi_cmp_si( const bi_ptr n1, const int n2) {
	BIGNUM *temp = BN_new();
	BN_set_word( temp, n2);
	int res = BN_cmp( n1, temp);
	BN_free( temp);
	return res;
}

/*  n1 == n2   return 1 (true)
 *  else return 0
*/
INLINE_DECL int bi_equals( const bi_ptr n1, const bi_ptr n2) {
	return BN_cmp( n1, n2) == 0 ? 1 :0;
}

/*  n1 == n2   return 1 (true)
 *  else return 0
*/
INLINE_DECL int bi_equals_si( const bi_ptr n1, const int n2) {
	return BN_is_word( n1, n2);
}

/***********************************************************************************
	CONVERSIONS
*************************************************************************************/

INLINE_DECL char *bi_2_hex_char(const bi_ptr i) {
	char *result = BN_bn2hex( i);

	if( result == NULL) {
		return NULL;
	}
	list_add( allocs, result);
	return result;
}

INLINE_DECL char *bi_2_dec_char(const bi_ptr i) {
	char *result = BN_bn2dec( i);

	if( result == NULL) {
		return NULL;
	}
	list_add( allocs, result);
	return result;
}

INLINE_DECL bi_ptr bi_set( bi_ptr result, const bi_ptr value) {
	BN_copy( result, value);
	return result;
}

INLINE_DECL bi_ptr bi_set_as_hex( bi_ptr result, const char *value) {
	BN_hex2bn( &result, value);
	return result;
}

INLINE_DECL bi_ptr bi_set_as_dec( bi_ptr result, const char *value) {
	BN_dec2bn( &result, value);
	return result;
}

/* set <i> with the value represented by unsigned int <value> */
 /*    <i> := <value>          */
INLINE_DECL bi_ptr bi_set_as_si( bi_ptr result, const int value) {
	if( value < 0) {
		BN_set_word( result, -value);
		result->neg=1;
	} else
		BN_set_word( result, value);
	return result;
}

/* return (long)bi_t  */
INLINE_DECL long bi_get_si(const bi_ptr i) {
	long result =  BN_get_word( i);

	if( i->neg == 1) {
		return -result;
	}
	return result;
}

/* return the size of a network byte order representation of <i>  */
INLINE_DECL long bi_nbin_size(const bi_ptr i) {
	return BN_num_bytes( i);
}

/* return a BYTE *  in network byte order - big endian - and update the length <length>  */
INLINE_DECL unsigned char *bi_2_nbin( int *length, const bi_ptr i) {
	unsigned char *ret;

	*length = BN_num_bytes( i);
	ret = (unsigned char *)bi_alloc( *length * 2);
	if( ret == NULL) return NULL;
	BN_bn2bin( i, ret);
	return ret;
}

/* return a BYTE * - in network byte order -  and update the length <length>  */
/* different from bi_2_nbin: you should reserve enough memory for the storage */
INLINE_DECL void bi_2_nbin1( int *length, unsigned char *buffer, const bi_ptr i) {
	*length = BN_num_bytes( i);
	BN_bn2bin( i, buffer);
}

/* return a bi_ptr that correspond to the big endian encoded BYTE array of length <n_length> */
INLINE_DECL bi_ptr bi_set_as_nbin( const unsigned long length, const unsigned char *buffer) {
	bi_ptr ret_bi = bi_new_ptr();

	if( ret_bi == NULL) return NULL;
	if( BN_bin2bn( buffer, length, ret_bi) == NULL) {
		bi_free( ret_bi);
		return NULL;
	}
	return ret_bi;
}

/* convert a bi to a openssl BIGNUM struct */
INLINE_DECL BIGNUM *bi_2_BIGNUM( const bi_ptr i) {
	return i;
}

/* set <i> with the value represented by the given openssl BIGNUM struct */
INLINE_DECL bi_ptr bi_set_as_BIGNUM( bi_ptr i, BIGNUM *bn) {
	return bi_set( i, bn);
}

/***********************************************************************************
	BITS OPERATION
*************************************************************************************/
/* set the bit to 1 */
INLINE_DECL bi_ptr bi_setbit(bi_ptr result, const int bit) {
	BN_set_bit( result, bit);
	return result;
}

/* <result> := <i> << <n> */
INLINE_DECL bi_ptr bi_shift_left( bi_ptr result, const bi_ptr i, const int n) {
	BN_lshift( result, i, n);
	return result;
}

/* <result> := <i> >> <n> */
INLINE_DECL bi_ptr bi_shift_right( bi_ptr result, const bi_ptr i, const int n) {
	BN_rshift( result, i, n);
	return result;
}

/* create a random of length <length> bits */
/*  res := random( length)  */
INLINE_DECL bi_ptr bi_urandom( bi_ptr result, const long length) {
	/*
	 *  <result> will be a  generated cryptographically strong pseudo-random number of length
	 *  <length>
	 */
	BN_rand( result, length, -1, 0);
	return result;
}


/* res := <n> mod <m> */
INLINE_DECL bi_ptr bi_mod_si( bi_ptr result, const bi_ptr n, const long m) {
	BIGNUM *mod = BN_new();
	BN_set_word( mod, m);
	BN_mod( result, n, mod, context);
	BN_free( mod);
	return result;
}

/* res := <n> mod <m> */
INLINE_DECL bi_ptr bi_mod( bi_ptr result, const bi_ptr n, const bi_ptr m) {
	BN_mod( result, n, m, context);
	if( result->neg == 1) {
		result->neg=0;
		BN_sub( result, m, result);
	}
	return result;
}

/* result := (inverse of <i>) mod <m> */
/* if the inverse exist, return >0, otherwise 0 */
INLINE_DECL int bi_invert_mod( bi_ptr result, const bi_ptr i, const bi_ptr m) {
	while( ERR_get_error() != 0);
	BN_mod_inverse( result, i, m, context);
	return ERR_get_error() == 0 ? 1 : 0;
}

/* generate a prime number of <length> bits  */
INLINE_DECL bi_ptr bi_generate_prime( bi_ptr result, const long bit_length) {
	BN_generate_prime(result, bit_length, 0, NULL, NULL, NULL, NULL);
	return result;
}

/* generate a safe prime number of <length> bits  */
/* by safe we mean a prime p so that (p-1)/2 is also prime */
INLINE_DECL bi_ptr bi_generate_safe_prime( bi_ptr result, const long bit_length) {
	BN_generate_prime(result, bit_length, 1, NULL, NULL, NULL, NULL);
	return result;
}

/* return in <result> the greatest common divisor of <a> and <b> */
/* <result> := gcd( <a>, <b>) */
INLINE_DECL bi_ptr bi_gcd( bi_ptr result, bi_ptr a, bi_ptr b) {
	BN_gcd( result, a, b, context);
	return result;
}


#endif /*BI_OPENSSL_*/
