/*	$NetBSD: msg_035.c,v 1.11 2022/06/15 20:18:31 rillig Exp $	*/
# 3 "msg_035.c"

// Test for message: illegal bit-field type '%s' [35]

/* Omit -g, see gcc_bit_field_types.c. */
/* lint1-flags: -Sw */

/*
 * In traditional C, only unsigned int is a portable bit-field type.
 *
 * In C90, only int, signed int and unsigned int are allowed (3.5.2.1p7).
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

	/* expect+1: warning: illegal bit-field type 'long' [35] */
	long long_flag: 1;
	/* expect+1: warning: illegal bit-field type 'unsigned long' [35] */
	unsigned long unsigned_long_flag: 1;
	/* expect+1: warning: illegal bit-field type 'long long' [35] */
	long long long_long_flag: 1;
	/* expect+1: warning: illegal bit-field type 'unsigned long long' [35] */
	unsigned long long unsigned_long_long_flag: 1;

	/* __int128_t omitted since it is not always defined */
	/* __uint128_t omitted since it is not always defined */

	/* expect+1: warning: illegal bit-field type 'float' [35] */
	float float_flag: 1;
	/* expect+1: warning: illegal bit-field type 'double' [35] */
	double double_flag: 1;
	/* expect+1: warning: illegal bit-field type 'long double' [35] */
	long double long_double_flag: 1;
	/* expect+2: error: void type for 'void_flag' [19] */
	/* expect+1: error: zero size bit-field [37] */
	void void_flag: 1;
	/* expect+1: warning: illegal bit-field type 'struct typedef example_struct' [35] */
	example_struct struct_flag: 1;
	/* expect+1: warning: illegal bit-field type 'union typedef example_union' [35] */
	example_union union_flag: 1;

	example_enum enum_flag: 1;

	/* expect+1: warning: illegal bit-field type 'pointer to void' [35] */
	void *pointer_flag: 1;
	/* expect+1: warning: illegal bit-field type 'array[4] of unsigned int' [35] */
	unsigned int array_flag[4]: 1;
	/* expect+1: warning: illegal bit-field type 'function(int, pointer to const char) returning void' [35] */
	example_function function_flag: 1;
	/* expect+2: error: invalid type for _Complex [308] */
	/* expect+1: warning: illegal bit-field type 'double _Complex' [35] */
	_Complex complex_flag: 1;
	/* expect+1: warning: illegal bit-field type 'float _Complex' [35] */
	float _Complex float_complex_flag: 1;
	/* expect+1: warning: illegal bit-field type 'double _Complex' [35] */
	double _Complex double_complex_flag: 1;
	/* expect+1: warning: illegal bit-field type 'long double _Complex' [35] */
	long double _Complex long_double_complex_flag: 1;
};
