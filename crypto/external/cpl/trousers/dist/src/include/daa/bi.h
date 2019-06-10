
/*
 * Licensed Materials - Property of IBM
 *
 * trousers - An open source TCG Software Stack
 *
 * (C) Copyright International Business Machines Corp. 2006
 *
 */

#ifndef BI_H_
#define BI_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// for the BIGNUM definition
#include <openssl/bn.h>

#include "list.h"

#define INLINE
#undef INLINE_DECL
#define INLINE_DECL static inline

void * (*bi_alloc)(size_t size);

// keep the list of allocated memory, usually used for the format functions
extern list_ptr allocs;

/************************************************************************************
	TYPE DEF
*************************************************************************************/

#ifdef BI_GMP
#include "bi_gmp.h"
#endif

#ifdef BI_OPENSSL
#include "bi_openssl.h"
#endif

/************************************************************************************
	TYPE DEF
*************************************************************************************/

struct _bi_array{
	bi_ptr *array;
	int length;
};

typedef struct _bi_array bi_array[1];
typedef struct _bi_array *bi_array_ptr;

/***********************************************************************************
	CONSTANT
*************************************************************************************/

extern bi_t bi_0;
extern bi_t bi_1;
extern bi_t bi_2;

/***********************************************************************************
	TEMPORARY (WORK)
*************************************************************************************/

/*
extern bi_t bi_tmp;
extern bi_t bi_tmp1;
extern bi_t bi_tmp2;
extern bi_t bi_tmp3;
extern bi_t bi_tmp4;
extern bi_t bi_tmp5;
extern bi_t bi_tmp6;
extern bi_t bi_tmp7;
extern bi_t bi_tmp8;
extern bi_t bi_tmp9;
*/

/***********************************************************************************
	MACROS
*************************************************************************************/
#define ALLOC_BI_ARRAY()  (bi_array_ptr)malloc( sizeof( bi_array))

#if 0
#define BI_SAVE( a, b)  do { bi_save( a, #a, b); } while(0);
#define BI_SAVE_ARRAY( a, b)  do { bi_save_array( a, #a, b); } while(0);
#define BI_LOAD( a, b)  do { bi_load( a, b); } while(0);
#define BI_LOAD_ARRAY( a, b)  do { bi_load_array( a, b); } while(0);
#endif

#ifdef BI_DEBUG
#define DUMP_BI(field)  do { \
	fprintf(stderr, "%s=%s [%ld]\n", #field, bi_2_hex_char( field), bi_nbin_size(field));\
	} while(0);

#define DUMP_BI_ARRAY(field)  do { dump_bi_array( #field, field); } while(0);

#else
#define DUMP_BI(field)

#define DUMP_BI_ARRAY(field)
#endif

/* to free only defines bi_ptr */
#define FREE_BI(a) do { if( (a) != NULL) bi_free_ptr( a); } while(0);

/***********************************************************************************
	DUMP LIB
*************************************************************************************/

char *dump_byte_array(int len, unsigned char *array);

/* convert <strings> and return it into a byte array <result> of length <length> */
unsigned char *retrieve_byte_array( int *len, const char *strings);

/***********************************************************************************
	LIBRARY MANAGEMENT
*************************************************************************************/
/*
 initialize the library
 bi_alloc_p allocation function used only for exporting a bi struct, so for bi_2_nbin
 if define as NULL, a stdlib malloc() will be used
*/
void bi_init( void * (*bi_alloc_p)(size_t size));

/* release resources used by the library */
void bi_release(void);

/* return >0 if the library was initialized */
int bi_is_initialized(void);

/* free the list of internally allocated memory, usually used for the format functions */
void bi_flush_memory(void);

/***********************************************************************************
	ALLOCATION & BASIC SETTINGS
*************************************************************************************/

/* create a big integer */
bi_ptr bi_new( bi_ptr result);

/* create a big integer pointer */
bi_ptr bi_new_ptr(void);

/* free resources allocated to the big integer <i> */
void bi_free(const bi_ptr i);

/* free resources allocated to the big integer pointer <i> */
void bi_free_ptr(const bi_ptr i);

/* return the current number of bits of the number */
long bi_length( const bi_ptr res);

/* create a <big integer> array */
void bi_new_array( bi_array_ptr array, const int length);

/* create a <big integer> array */
void bi_new_array2( bi_array_ptr array, const int length);

/* free resources allocated to the big integer <i> */
void bi_free_array(bi_array_ptr array);

/* copy length pointers from the array <src, offset_src> to array <dest, offset_dest> */
void bi_copy_array(bi_array_ptr src,
		int offset_src,
		bi_array_ptr dest,
		int offset_dest,
		int length);

// for debugging
void dump_bi_array( char *field, const bi_array_ptr array);

/***********************************************************************************
	SAFE RANDOM
*************************************************************************************/

bi_ptr compute_random_number( bi_ptr result, const bi_ptr element);

#if 0
/***********************************************************************************
	SAVE / LOAD
*************************************************************************************/

/* load an big integer in the already open ("r")  file */
void bi_load( bi_ptr bi, FILE *file);

/* load an big integer array  in the already open ("r")  file */
void bi_load_array( bi_array_ptr array, FILE *file);

/* save an big integer array  in the already open ("w")  file */
void bi_save_array( const bi_array_ptr array, const char *name, FILE *file);

/* save an big integer in the already open ("w")  file */
void bi_save( const bi_ptr bi,const char *name,  FILE *file);
#endif

/***********************************************************************************
	CONVERSION
*************************************************************************************/

/* dump the big integer as hexadecimal  */
char *bi_2_hex_char(const bi_ptr i);

 /* dump the big integer as decimal  */
char *bi_2_dec_char(const bi_ptr i);

 /* set <i> to the same value as <value> */
 /*    <i> := <value>          */
bi_ptr bi_set( bi_ptr i, const bi_ptr value);

/* set <i> with the value represented by given hexadecimal <value> */
 /*    <i> := <value>          */
bi_ptr bi_set_as_hex( bi_ptr i, const char *value);

/* set <i> with the value represented by given decimal <value> */
 /*    <i> := <value>          */
bi_ptr bi_set_as_dec( bi_ptr i, const char *value);

/* set <i> with the value represented by unsigned int <value> */
 /*    <i> := <value>          */
bi_ptr bi_set_as_si( bi_ptr result, const int value);

/* return (long)bi_t  */
long bi_get_si(const bi_ptr i);

/* return the size of a network byte order representation of <i>  */
long bi_nbin_size(const bi_ptr i);

/* return a BYTE * - in network byte order -  and update the length <length>  */
/* the result is allocated internally */
unsigned char *bi_2_nbin( int *length, const bi_ptr i);

/* return a BYTE * - in network byte order -  and update the length <length>  */
/* different from bi_2_nbin: you should reserve enough memory for the storage */
void bi_2_nbin1( int *length, unsigned char *, const bi_ptr i);

/* return a bi_ptr that correspond to the big endian encoded BYTE array of length <n_length> */
bi_ptr bi_set_as_nbin( const unsigned long length, const unsigned char *buffer);

/*
 convert <bi> to a byte array of length result,
 the beginning of this buffer is feel with '0' if needed
*/
void bi_2_byte_array( unsigned char *result, int length, bi_ptr bi);

/* convert a bi to a openssl BIGNUM struct */
BIGNUM *bi_2_BIGNUM( const bi_ptr);


/***********************************************************************************
	BITS OPERATION
*************************************************************************************/
/* set the bit to 1 */
bi_ptr bi_setbit( bi_ptr result, const int bit);

/* <result> := <i> << <n> */
bi_ptr bi_shift_left( bi_ptr result, const bi_ptr i, const int n);

/* <result> := <i> >> <n> */
bi_ptr bi_shift_right( bi_ptr result, const bi_ptr i, const int n);

/***********************************************************************************
	NUMBER THEORIE OPERATION
*************************************************************************************/
/* create a random of length <length> bits */
/*  res := random( length)  */
bi_ptr bi_urandom( bi_ptr res, const long length);

/* res := <n> mod <m> */
bi_ptr bi_mod(bi_ptr res, const bi_ptr n, const bi_ptr m);

/* res := <n> mod <m> */
bi_ptr bi_mod_si(bi_ptr res, const bi_ptr n, const long m);

/* generate prime number of <length> bits  */
bi_ptr bi_generate_prime( bi_ptr i, const long length);

/*
return true (>0, bigger is better, but this is contextual to the plugin)
if <i> is a probably prime
*/
int bi_is_probable_prime( const bi_ptr i);

/* result := (inverse of <i>) mod <m> */
/* if the inverse exist, return >0, otherwise 0 */
int bi_invert_mod( bi_ptr result, const bi_ptr i, const bi_ptr m);

/* generate a safe prime number of <length> bits  */
/* by safe we mean a prime p so that (p-1)/2 is also prime */
bi_ptr bi_generate_safe_prime( bi_ptr result, const long bit_length);

/* return in <result> the greatest common divisor of <a> and <b> */
/* <result> := gcd( <a>, <b>) */
bi_ptr bi_gcd( bi_ptr result, bi_ptr a, bi_ptr b);

/***********************************************************************************
	BASIC MATH OPERATION
*************************************************************************************/

/* <result> := result++ */
bi_ptr bi_inc(bi_ptr result);

/* <result> := result-- */
bi_ptr bi_dec(bi_ptr result);

/* <result> := - <result> */
bi_ptr bi_negate( bi_ptr result);

/* set <result> by the multiplication of <i> by the long <n>  */
/*  <result> := <i> * <n>   */
bi_ptr bi_mul_si( bi_ptr result, const bi_ptr i, const long n);

/*  <result> := <i> * <n>   */
bi_ptr bi_mul( bi_ptr result, const bi_ptr i, const bi_ptr n);

/* set <result> by the division of <i> by the long <n>  */
/*  <result> := <i> / <n>   */
bi_ptr bi_div_si( bi_ptr result, const bi_ptr i, const long n);

/*  <result> := <i> / <n>   */
bi_ptr bi_div( bi_ptr result, const bi_ptr i, const bi_ptr n);

/* set <result> by the addition of <i> by the long <n>  */
/*  <result> := <i> + <n>   */
bi_ptr bi_add_si( bi_ptr result, const bi_ptr i, const long n);

/*  <result> := <i> + <n>  */
bi_ptr bi_add( bi_ptr result, const bi_ptr i, const bi_ptr n);

/*  <result> := <i> - <n>   */
bi_ptr bi_sub_si( bi_ptr result, const bi_ptr i, const long n);

/*  <result> := <i> - <n>  */
bi_ptr bi_sub( bi_ptr result, const bi_ptr i, const bi_ptr n);

/*  <result> := ( <g> ^ <e> ) mod <m>  */
bi_ptr bi_mod_exp_si( bi_ptr result, const bi_ptr g, const bi_ptr e, const long m);

/*  <result> := ( <g> ^ <e> ) mod <m>  */
bi_ptr bi_mod_exp( bi_ptr result, const bi_ptr g, const bi_ptr e, const bi_ptr m);

/*
multiple-exponentiation
<result> := mod( Multi( <g>i, <e>i), number of byte <m>) with  0 <= i <= <n>
bi_t[] is used for commodity (bi-ptr[] need allocation for each bi_ptr, something made
in the stack with bi_t)
*/
bi_ptr bi_multi_mod_exp( bi_ptr result,
			const int n,
			const bi_t g[],
			const long e[],
			const int m);

/***********************************************************************************
	COMPARAISON
*************************************************************************************/
/*	n1<n2   return negative value
	n1 = n2 return 0
	n1>n2   return positive value
*/
int bi_cmp( const bi_ptr n1, const bi_ptr n2);

/*	n1<n2   return negative value
	n1 = n2 return 0
	n1>n2   return positive value
*/
int bi_cmp_si( const bi_ptr n1, const int n2);

/*	n1 == n2   return 1 (true)
	else return 0
*/
int bi_equals( const bi_ptr n1, const bi_ptr n2);

/*	n1 == n2   return 1 (true)
	else return 0
*/
int bi_equals_si( const bi_ptr n1, const int n2);

#endif /*BI_H_*/
