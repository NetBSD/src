/*	$NetBSD: msg_035.c,v 1.6 2021/01/31 11:12:07 rillig Exp $	*/
# 3 "msg_035.c"

// Test for message: illegal bit-field type [35]

/*
 * In traditional C, only unsigned int is a portable bit-field type.
 *
 * In C89, only int, signed int and unsigned int are allowed (3.5.2.1p7).
 *
 * In C99 and C11, only _Bool, signed int and unsigned int are allowed,
 * plus implementation-defined types (6.7.2.1p5).
 */

typedef struct {
	int dummy;
} example_struct;

typedef union {
	int dummy;
} example_union;

typedef enum {
	NO, YES
} example_enum;

typedef void (example_function)(int, const char *);

/* Try all types from tspec_t. */
struct example {
	signed signed_flag: 1;
	unsigned unsigned_flag: 1;
	_Bool boolean_flag: 1;
	char char_flag: 1;
	signed char signed_char_flag: 1;
	unsigned char unsigned_char_flag: 1;
	short short_flag: 1;
	unsigned short unsigned_short_flag: 1;
	int int_flag: 1;
	unsigned int unsigned_int_flag: 1;
	long long_flag: 1;				/* expect: 35 */
	unsigned long unsigned_long_flag: 1;		/* expect: 35 */
	long long long_long_flag: 1;			/* expect: 35 */
	unsigned long long unsigned_long_long_flag: 1;	/* expect: 35 */
	/* __int128_t omitted since it is not always defined */
	/* __uint128_t omitted since it is not always defined */
	float float_flag: 1;				/* expect: 35 */
	double double_flag: 1;				/* expect: 35 */
	long double long_double_flag: 1;		/* expect: 35 */
	void void_flag: 1;				/* expect: 19, 37 */
	example_struct struct_flag: 1;			/* expect: 35 */
	example_union union_flag: 1;			/* expect: 35 */
	example_enum enum_flag: 1;
	void *pointer_flag: 1;				/* expect: 35 */
	unsigned int array_flag[4]: 1;			/* expect: 35 */
	example_function function_flag: 1;		/* expect: 35 */
	_Complex complex_flag: 1;			/* expect: 35, 308 */
	float _Complex float_complex_flag: 1;		/* expect: 35 */
	double _Complex double_complex_flag: 1;		/* expect: 35 */
	long double _Complex long_double_complex_flag: 1; /* expect: 35 */
};
