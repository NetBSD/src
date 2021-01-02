/*	$NetBSD: msg_035.c,v 1.2 2021/01/02 15:55:54 rillig Exp $	*/
# 3 "msg_035.c"

// Test for message: illegal bit-field type [35]

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
	_Bool boolean_flag: 1;		// FIXME: allowed since C99 6.7.2.1p5
	char char_flag: 1;
	signed char signed_char_flag: 1;
	unsigned char unsigned_char_flag: 1;
	short short_flag: 1;
	unsigned short unsigned_short_flag: 1;
	int int_flag: 1;
	unsigned int unsigned_int_flag: 1;
	long long_flag: 1;
	unsigned long unsigned_long_flag: 1;
	long long long_long_flag: 1;
	unsigned long long unsigned_long_long_flag: 1;
	/* __int128_t omitted since it is not always defined */
	/* __uint128_t omitted since it is not always defined */
	float float_flag: 1;
	double double_flag: 1;
	long double long_double_flag: 1;
	void void_flag: 1;
	example_struct struct_flag: 1;
	example_union union_flag: 1;
	example_enum enum_flag: 1;
	void *pointer_flag: 1;
	unsigned int array_flag[4]: 1;
	example_function function_flag: 1;
// FIXME: aborts:	_Complex complex_flag: 1;
	_Complex complex_flag: 1;
	float _Complex float_complex_flag: 1;
	double _Complex double_complex_flag: 1;
	long double _Complex long_double_complex_flag: 1;
};
