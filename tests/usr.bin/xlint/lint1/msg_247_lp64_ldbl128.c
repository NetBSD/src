/*	$NetBSD: msg_247_lp64_ldbl128.c,v 1.4 2023/07/08 16:13:00 rillig Exp $	*/
# 3 "msg_247_lp64_ldbl128.c"

// Test for message: pointer cast from '%s' to '%s' may be troublesome [247]

// In non-portable mode, lint warns based on the actual type sizes.
//
// See also:
//	msg_247_ilp32_ldbl64.c
//	msg_247_portable.c

/* lint1-only-if: lp64 ldbl128 */
/* lint1-extra-flags: -c -X 351 */

typedef double double_array[5];
typedef struct {
	char member;
} char_struct;
typedef struct {
	double member;
} double_struct;
typedef union {
	char member;
} char_union;
typedef union {
	double member;
} double_union;
typedef enum {
	CONSTANT
} int_enum;
typedef void (*function_pointer)(void);

static _Bool *bool_ptr;
static char *char_ptr;
static signed char *schar_ptr;
static unsigned char *uchar_ptr;
static short *short_ptr;
static unsigned short *ushort_ptr;
static int *int_ptr;
static unsigned int *uint_ptr;
static long *long_ptr;
static unsigned long *ulong_ptr;
static long long *llong_ptr;
static unsigned long long *ullong_ptr;
// No int128_t, as that is only supported on LP64 platforms.
static float *float_ptr;
static double *double_ptr;
static long double *ldouble_ptr;
static float _Complex *fcomplex_ptr;
static double _Complex *dcomplex_ptr;
static long double _Complex *lcomplex_ptr;
static void *void_ptr;
static char_struct *char_struct_ptr;
static double_struct *double_struct_ptr;
static char_union *char_union_ptr;
static double_union *double_union_ptr;
static int_enum *enum_ptr;
static double_array *double_array_ptr;
static function_pointer func_ptr;

void
all_casts(void)
{
	bool_ptr = (typeof(bool_ptr))bool_ptr;
	bool_ptr = (typeof(bool_ptr))char_ptr;
	/* expect+1: warning: pointer cast from 'pointer to signed char' to 'pointer to _Bool' may be troublesome [247] */
	bool_ptr = (typeof(bool_ptr))schar_ptr;
	bool_ptr = (typeof(bool_ptr))uchar_ptr;
	/* expect+1: warning: pointer cast from 'pointer to short' to 'pointer to _Bool' may be troublesome [247] */
	bool_ptr = (typeof(bool_ptr))short_ptr;
	/* expect+1: warning: pointer cast from 'pointer to unsigned short' to 'pointer to _Bool' may be troublesome [247] */
	bool_ptr = (typeof(bool_ptr))ushort_ptr;
	/* expect+1: warning: pointer cast from 'pointer to int' to 'pointer to _Bool' may be troublesome [247] */
	bool_ptr = (typeof(bool_ptr))int_ptr;
	/* expect+1: warning: pointer cast from 'pointer to unsigned int' to 'pointer to _Bool' may be troublesome [247] */
	bool_ptr = (typeof(bool_ptr))uint_ptr;
	/* expect+1: warning: pointer cast from 'pointer to long' to 'pointer to _Bool' may be troublesome [247] */
	bool_ptr = (typeof(bool_ptr))long_ptr;
	/* expect+1: warning: pointer cast from 'pointer to unsigned long' to 'pointer to _Bool' may be troublesome [247] */
	bool_ptr = (typeof(bool_ptr))ulong_ptr;
	/* expect+1: warning: pointer cast from 'pointer to long long' to 'pointer to _Bool' may be troublesome [247] */
	bool_ptr = (typeof(bool_ptr))llong_ptr;
	/* expect+1: warning: pointer cast from 'pointer to unsigned long long' to 'pointer to _Bool' may be troublesome [247] */
	bool_ptr = (typeof(bool_ptr))ullong_ptr;
	/* expect+1: warning: pointer cast from 'pointer to float' to 'pointer to _Bool' may be troublesome [247] */
	bool_ptr = (typeof(bool_ptr))float_ptr;
	/* expect+1: warning: pointer cast from 'pointer to double' to 'pointer to _Bool' may be troublesome [247] */
	bool_ptr = (typeof(bool_ptr))double_ptr;
	/* expect+1: warning: pointer cast from 'pointer to long double' to 'pointer to _Bool' may be troublesome [247] */
	bool_ptr = (typeof(bool_ptr))ldouble_ptr;
	/* expect+1: warning: pointer cast from 'pointer to float _Complex' to 'pointer to _Bool' may be troublesome [247] */
	bool_ptr = (typeof(bool_ptr))fcomplex_ptr;
	/* expect+1: warning: pointer cast from 'pointer to double _Complex' to 'pointer to _Bool' may be troublesome [247] */
	bool_ptr = (typeof(bool_ptr))dcomplex_ptr;
	/* expect+1: warning: pointer cast from 'pointer to long double _Complex' to 'pointer to _Bool' may be troublesome [247] */
	bool_ptr = (typeof(bool_ptr))lcomplex_ptr;
	bool_ptr = (typeof(bool_ptr))void_ptr;
	/* expect+1: warning: pointer cast from 'pointer to struct typedef char_struct' to 'pointer to _Bool' may be troublesome [247] */
	bool_ptr = (typeof(bool_ptr))char_struct_ptr;
	/* expect+1: warning: pointer cast from 'pointer to struct typedef double_struct' to 'pointer to _Bool' may be troublesome [247] */
	bool_ptr = (typeof(bool_ptr))double_struct_ptr;
	/* expect+1: warning: pointer cast from 'pointer to union typedef char_union' to 'pointer to _Bool' may be troublesome [247] */
	bool_ptr = (typeof(bool_ptr))char_union_ptr;
	/* expect+1: warning: pointer cast from 'pointer to union typedef double_union' to 'pointer to _Bool' may be troublesome [247] */
	bool_ptr = (typeof(bool_ptr))double_union_ptr;
	/* expect+1: warning: pointer cast from 'pointer to enum typedef int_enum' to 'pointer to _Bool' may be troublesome [247] */
	bool_ptr = (typeof(bool_ptr))enum_ptr;
	/* expect+1: warning: pointer cast from 'pointer to array[5] of double' to 'pointer to _Bool' may be troublesome [247] */
	bool_ptr = (typeof(bool_ptr))double_array_ptr;
	/* expect+1: warning: converting 'pointer to function(void) returning void' to 'pointer to _Bool' is questionable [229] */
	bool_ptr = (typeof(bool_ptr))func_ptr;

	char_ptr = (typeof(char_ptr))bool_ptr;
	char_ptr = (typeof(char_ptr))char_ptr;
	char_ptr = (typeof(char_ptr))schar_ptr;
	char_ptr = (typeof(char_ptr))uchar_ptr;
	char_ptr = (typeof(char_ptr))short_ptr;
	char_ptr = (typeof(char_ptr))ushort_ptr;
	char_ptr = (typeof(char_ptr))int_ptr;
	char_ptr = (typeof(char_ptr))uint_ptr;
	char_ptr = (typeof(char_ptr))long_ptr;
	char_ptr = (typeof(char_ptr))ulong_ptr;
	char_ptr = (typeof(char_ptr))llong_ptr;
	char_ptr = (typeof(char_ptr))ullong_ptr;
	char_ptr = (typeof(char_ptr))float_ptr;
	char_ptr = (typeof(char_ptr))double_ptr;
	char_ptr = (typeof(char_ptr))ldouble_ptr;
	char_ptr = (typeof(char_ptr))fcomplex_ptr;
	char_ptr = (typeof(char_ptr))dcomplex_ptr;
	char_ptr = (typeof(char_ptr))lcomplex_ptr;
	char_ptr = (typeof(char_ptr))void_ptr;
	char_ptr = (typeof(char_ptr))char_struct_ptr;
	char_ptr = (typeof(char_ptr))double_struct_ptr;
	char_ptr = (typeof(char_ptr))char_union_ptr;
	char_ptr = (typeof(char_ptr))double_union_ptr;
	char_ptr = (typeof(char_ptr))enum_ptr;
	char_ptr = (typeof(char_ptr))double_array_ptr;
	/* expect+1: warning: converting 'pointer to function(void) returning void' to 'pointer to char' is questionable [229] */
	char_ptr = (typeof(char_ptr))func_ptr;

	/* expect+1: warning: pointer cast from 'pointer to _Bool' to 'pointer to signed char' may be troublesome [247] */
	schar_ptr = (typeof(schar_ptr))bool_ptr;
	schar_ptr = (typeof(schar_ptr))char_ptr;
	schar_ptr = (typeof(schar_ptr))schar_ptr;
	schar_ptr = (typeof(schar_ptr))uchar_ptr;
	/* expect+1: warning: pointer cast from 'pointer to short' to 'pointer to signed char' may be troublesome [247] */
	schar_ptr = (typeof(schar_ptr))short_ptr;
	/* expect+1: warning: pointer cast from 'pointer to unsigned short' to 'pointer to signed char' may be troublesome [247] */
	schar_ptr = (typeof(schar_ptr))ushort_ptr;
	/* expect+1: warning: pointer cast from 'pointer to int' to 'pointer to signed char' may be troublesome [247] */
	schar_ptr = (typeof(schar_ptr))int_ptr;
	/* expect+1: warning: pointer cast from 'pointer to unsigned int' to 'pointer to signed char' may be troublesome [247] */
	schar_ptr = (typeof(schar_ptr))uint_ptr;
	/* expect+1: warning: pointer cast from 'pointer to long' to 'pointer to signed char' may be troublesome [247] */
	schar_ptr = (typeof(schar_ptr))long_ptr;
	/* expect+1: warning: pointer cast from 'pointer to unsigned long' to 'pointer to signed char' may be troublesome [247] */
	schar_ptr = (typeof(schar_ptr))ulong_ptr;
	/* expect+1: warning: pointer cast from 'pointer to long long' to 'pointer to signed char' may be troublesome [247] */
	schar_ptr = (typeof(schar_ptr))llong_ptr;
	/* expect+1: warning: pointer cast from 'pointer to unsigned long long' to 'pointer to signed char' may be troublesome [247] */
	schar_ptr = (typeof(schar_ptr))ullong_ptr;
	/* expect+1: warning: pointer cast from 'pointer to float' to 'pointer to signed char' may be troublesome [247] */
	schar_ptr = (typeof(schar_ptr))float_ptr;
	/* expect+1: warning: pointer cast from 'pointer to double' to 'pointer to signed char' may be troublesome [247] */
	schar_ptr = (typeof(schar_ptr))double_ptr;
	/* expect+1: warning: pointer cast from 'pointer to long double' to 'pointer to signed char' may be troublesome [247] */
	schar_ptr = (typeof(schar_ptr))ldouble_ptr;
	/* expect+1: warning: pointer cast from 'pointer to float _Complex' to 'pointer to signed char' may be troublesome [247] */
	schar_ptr = (typeof(schar_ptr))fcomplex_ptr;
	/* expect+1: warning: pointer cast from 'pointer to double _Complex' to 'pointer to signed char' may be troublesome [247] */
	schar_ptr = (typeof(schar_ptr))dcomplex_ptr;
	/* expect+1: warning: pointer cast from 'pointer to long double _Complex' to 'pointer to signed char' may be troublesome [247] */
	schar_ptr = (typeof(schar_ptr))lcomplex_ptr;
	schar_ptr = (typeof(schar_ptr))void_ptr;
	/* expect+1: warning: pointer cast from 'pointer to struct typedef char_struct' to 'pointer to signed char' may be troublesome [247] */
	schar_ptr = (typeof(schar_ptr))char_struct_ptr;
	/* expect+1: warning: pointer cast from 'pointer to struct typedef double_struct' to 'pointer to signed char' may be troublesome [247] */
	schar_ptr = (typeof(schar_ptr))double_struct_ptr;
	/* expect+1: warning: pointer cast from 'pointer to union typedef char_union' to 'pointer to signed char' may be troublesome [247] */
	schar_ptr = (typeof(schar_ptr))char_union_ptr;
	/* expect+1: warning: pointer cast from 'pointer to union typedef double_union' to 'pointer to signed char' may be troublesome [247] */
	schar_ptr = (typeof(schar_ptr))double_union_ptr;
	/* expect+1: warning: pointer cast from 'pointer to enum typedef int_enum' to 'pointer to signed char' may be troublesome [247] */
	schar_ptr = (typeof(schar_ptr))enum_ptr;
	/* expect+1: warning: pointer cast from 'pointer to array[5] of double' to 'pointer to signed char' may be troublesome [247] */
	schar_ptr = (typeof(schar_ptr))double_array_ptr;
	/* expect+1: warning: converting 'pointer to function(void) returning void' to 'pointer to signed char' is questionable [229] */
	schar_ptr = (typeof(schar_ptr))func_ptr;

	uchar_ptr = (typeof(uchar_ptr))bool_ptr;
	uchar_ptr = (typeof(uchar_ptr))char_ptr;
	uchar_ptr = (typeof(uchar_ptr))schar_ptr;
	uchar_ptr = (typeof(uchar_ptr))uchar_ptr;
	uchar_ptr = (typeof(uchar_ptr))short_ptr;
	uchar_ptr = (typeof(uchar_ptr))ushort_ptr;
	uchar_ptr = (typeof(uchar_ptr))int_ptr;
	uchar_ptr = (typeof(uchar_ptr))uint_ptr;
	uchar_ptr = (typeof(uchar_ptr))long_ptr;
	uchar_ptr = (typeof(uchar_ptr))ulong_ptr;
	uchar_ptr = (typeof(uchar_ptr))llong_ptr;
	uchar_ptr = (typeof(uchar_ptr))ullong_ptr;
	uchar_ptr = (typeof(uchar_ptr))float_ptr;
	uchar_ptr = (typeof(uchar_ptr))double_ptr;
	uchar_ptr = (typeof(uchar_ptr))ldouble_ptr;
	uchar_ptr = (typeof(uchar_ptr))fcomplex_ptr;
	uchar_ptr = (typeof(uchar_ptr))dcomplex_ptr;
	uchar_ptr = (typeof(uchar_ptr))lcomplex_ptr;
	uchar_ptr = (typeof(uchar_ptr))void_ptr;
	uchar_ptr = (typeof(uchar_ptr))char_struct_ptr;
	uchar_ptr = (typeof(uchar_ptr))double_struct_ptr;
	uchar_ptr = (typeof(uchar_ptr))char_union_ptr;
	uchar_ptr = (typeof(uchar_ptr))double_union_ptr;
	uchar_ptr = (typeof(uchar_ptr))enum_ptr;
	uchar_ptr = (typeof(uchar_ptr))double_array_ptr;
	/* expect+1: warning: converting 'pointer to function(void) returning void' to 'pointer to unsigned char' is questionable [229] */
	uchar_ptr = (typeof(uchar_ptr))func_ptr;

	/* expect+1: warning: pointer cast from 'pointer to _Bool' to 'pointer to short' may be troublesome [247] */
	short_ptr = (typeof(short_ptr))bool_ptr;
	short_ptr = (typeof(short_ptr))char_ptr;
	/* expect+1: warning: pointer cast from 'pointer to signed char' to 'pointer to short' may be troublesome [247] */
	short_ptr = (typeof(short_ptr))schar_ptr;
	short_ptr = (typeof(short_ptr))uchar_ptr;
	short_ptr = (typeof(short_ptr))short_ptr;
	short_ptr = (typeof(short_ptr))ushort_ptr;
	/* expect+1: warning: pointer cast from 'pointer to int' to 'pointer to short' may be troublesome [247] */
	short_ptr = (typeof(short_ptr))int_ptr;
	/* expect+1: warning: pointer cast from 'pointer to unsigned int' to 'pointer to short' may be troublesome [247] */
	short_ptr = (typeof(short_ptr))uint_ptr;
	/* expect+1: warning: pointer cast from 'pointer to long' to 'pointer to short' may be troublesome [247] */
	short_ptr = (typeof(short_ptr))long_ptr;
	/* expect+1: warning: pointer cast from 'pointer to unsigned long' to 'pointer to short' may be troublesome [247] */
	short_ptr = (typeof(short_ptr))ulong_ptr;
	/* expect+1: warning: pointer cast from 'pointer to long long' to 'pointer to short' may be troublesome [247] */
	short_ptr = (typeof(short_ptr))llong_ptr;
	/* expect+1: warning: pointer cast from 'pointer to unsigned long long' to 'pointer to short' may be troublesome [247] */
	short_ptr = (typeof(short_ptr))ullong_ptr;
	/* expect+1: warning: pointer cast from 'pointer to float' to 'pointer to short' may be troublesome [247] */
	short_ptr = (typeof(short_ptr))float_ptr;
	/* expect+1: warning: pointer cast from 'pointer to double' to 'pointer to short' may be troublesome [247] */
	short_ptr = (typeof(short_ptr))double_ptr;
	/* expect+1: warning: pointer cast from 'pointer to long double' to 'pointer to short' may be troublesome [247] */
	short_ptr = (typeof(short_ptr))ldouble_ptr;
	/* expect+1: warning: pointer cast from 'pointer to float _Complex' to 'pointer to short' may be troublesome [247] */
	short_ptr = (typeof(short_ptr))fcomplex_ptr;
	/* expect+1: warning: pointer cast from 'pointer to double _Complex' to 'pointer to short' may be troublesome [247] */
	short_ptr = (typeof(short_ptr))dcomplex_ptr;
	/* expect+1: warning: pointer cast from 'pointer to long double _Complex' to 'pointer to short' may be troublesome [247] */
	short_ptr = (typeof(short_ptr))lcomplex_ptr;
	short_ptr = (typeof(short_ptr))void_ptr;
	/* expect+1: warning: pointer cast from 'pointer to struct typedef char_struct' to 'pointer to short' may be troublesome [247] */
	short_ptr = (typeof(short_ptr))char_struct_ptr;
	/* expect+1: warning: pointer cast from 'pointer to struct typedef double_struct' to 'pointer to short' may be troublesome [247] */
	short_ptr = (typeof(short_ptr))double_struct_ptr;
	/* expect+1: warning: pointer cast from 'pointer to union typedef char_union' to 'pointer to short' may be troublesome [247] */
	short_ptr = (typeof(short_ptr))char_union_ptr;
	/* expect+1: warning: pointer cast from 'pointer to union typedef double_union' to 'pointer to short' may be troublesome [247] */
	short_ptr = (typeof(short_ptr))double_union_ptr;
	/* expect+1: warning: pointer cast from 'pointer to enum typedef int_enum' to 'pointer to short' may be troublesome [247] */
	short_ptr = (typeof(short_ptr))enum_ptr;
	/* expect+1: warning: pointer cast from 'pointer to array[5] of double' to 'pointer to short' may be troublesome [247] */
	short_ptr = (typeof(short_ptr))double_array_ptr;
	/* expect+1: warning: converting 'pointer to function(void) returning void' to 'pointer to short' is questionable [229] */
	short_ptr = (typeof(short_ptr))func_ptr;

	/* expect+1: warning: pointer cast from 'pointer to _Bool' to 'pointer to unsigned short' may be troublesome [247] */
	ushort_ptr = (typeof(ushort_ptr))bool_ptr;
	ushort_ptr = (typeof(ushort_ptr))char_ptr;
	/* expect+1: warning: pointer cast from 'pointer to signed char' to 'pointer to unsigned short' may be troublesome [247] */
	ushort_ptr = (typeof(ushort_ptr))schar_ptr;
	ushort_ptr = (typeof(ushort_ptr))uchar_ptr;
	ushort_ptr = (typeof(ushort_ptr))short_ptr;
	ushort_ptr = (typeof(ushort_ptr))ushort_ptr;
	/* expect+1: warning: pointer cast from 'pointer to int' to 'pointer to unsigned short' may be troublesome [247] */
	ushort_ptr = (typeof(ushort_ptr))int_ptr;
	/* expect+1: warning: pointer cast from 'pointer to unsigned int' to 'pointer to unsigned short' may be troublesome [247] */
	ushort_ptr = (typeof(ushort_ptr))uint_ptr;
	/* expect+1: warning: pointer cast from 'pointer to long' to 'pointer to unsigned short' may be troublesome [247] */
	ushort_ptr = (typeof(ushort_ptr))long_ptr;
	/* expect+1: warning: pointer cast from 'pointer to unsigned long' to 'pointer to unsigned short' may be troublesome [247] */
	ushort_ptr = (typeof(ushort_ptr))ulong_ptr;
	/* expect+1: warning: pointer cast from 'pointer to long long' to 'pointer to unsigned short' may be troublesome [247] */
	ushort_ptr = (typeof(ushort_ptr))llong_ptr;
	/* expect+1: warning: pointer cast from 'pointer to unsigned long long' to 'pointer to unsigned short' may be troublesome [247] */
	ushort_ptr = (typeof(ushort_ptr))ullong_ptr;
	/* expect+1: warning: pointer cast from 'pointer to float' to 'pointer to unsigned short' may be troublesome [247] */
	ushort_ptr = (typeof(ushort_ptr))float_ptr;
	/* expect+1: warning: pointer cast from 'pointer to double' to 'pointer to unsigned short' may be troublesome [247] */
	ushort_ptr = (typeof(ushort_ptr))double_ptr;
	/* expect+1: warning: pointer cast from 'pointer to long double' to 'pointer to unsigned short' may be troublesome [247] */
	ushort_ptr = (typeof(ushort_ptr))ldouble_ptr;
	/* expect+1: warning: pointer cast from 'pointer to float _Complex' to 'pointer to unsigned short' may be troublesome [247] */
	ushort_ptr = (typeof(ushort_ptr))fcomplex_ptr;
	/* expect+1: warning: pointer cast from 'pointer to double _Complex' to 'pointer to unsigned short' may be troublesome [247] */
	ushort_ptr = (typeof(ushort_ptr))dcomplex_ptr;
	/* expect+1: warning: pointer cast from 'pointer to long double _Complex' to 'pointer to unsigned short' may be troublesome [247] */
	ushort_ptr = (typeof(ushort_ptr))lcomplex_ptr;
	ushort_ptr = (typeof(ushort_ptr))void_ptr;
	/* expect+1: warning: pointer cast from 'pointer to struct typedef char_struct' to 'pointer to unsigned short' may be troublesome [247] */
	ushort_ptr = (typeof(ushort_ptr))char_struct_ptr;
	/* expect+1: warning: pointer cast from 'pointer to struct typedef double_struct' to 'pointer to unsigned short' may be troublesome [247] */
	ushort_ptr = (typeof(ushort_ptr))double_struct_ptr;
	/* expect+1: warning: pointer cast from 'pointer to union typedef char_union' to 'pointer to unsigned short' may be troublesome [247] */
	ushort_ptr = (typeof(ushort_ptr))char_union_ptr;
	/* expect+1: warning: pointer cast from 'pointer to union typedef double_union' to 'pointer to unsigned short' may be troublesome [247] */
	ushort_ptr = (typeof(ushort_ptr))double_union_ptr;
	/* expect+1: warning: pointer cast from 'pointer to enum typedef int_enum' to 'pointer to unsigned short' may be troublesome [247] */
	ushort_ptr = (typeof(ushort_ptr))enum_ptr;
	/* expect+1: warning: pointer cast from 'pointer to array[5] of double' to 'pointer to unsigned short' may be troublesome [247] */
	ushort_ptr = (typeof(ushort_ptr))double_array_ptr;
	/* expect+1: warning: converting 'pointer to function(void) returning void' to 'pointer to unsigned short' is questionable [229] */
	ushort_ptr = (typeof(ushort_ptr))func_ptr;

	/* expect+1: warning: pointer cast from 'pointer to _Bool' to 'pointer to int' may be troublesome [247] */
	int_ptr = (typeof(int_ptr))bool_ptr;
	int_ptr = (typeof(int_ptr))char_ptr;
	/* expect+1: warning: pointer cast from 'pointer to signed char' to 'pointer to int' may be troublesome [247] */
	int_ptr = (typeof(int_ptr))schar_ptr;
	int_ptr = (typeof(int_ptr))uchar_ptr;
	/* expect+1: warning: pointer cast from 'pointer to short' to 'pointer to int' may be troublesome [247] */
	int_ptr = (typeof(int_ptr))short_ptr;
	/* expect+1: warning: pointer cast from 'pointer to unsigned short' to 'pointer to int' may be troublesome [247] */
	int_ptr = (typeof(int_ptr))ushort_ptr;
	int_ptr = (typeof(int_ptr))int_ptr;
	int_ptr = (typeof(int_ptr))uint_ptr;
	/* expect+1: warning: pointer cast from 'pointer to long' to 'pointer to int' may be troublesome [247] */
	int_ptr = (typeof(int_ptr))long_ptr;
	/* expect+1: warning: pointer cast from 'pointer to unsigned long' to 'pointer to int' may be troublesome [247] */
	int_ptr = (typeof(int_ptr))ulong_ptr;
	/* expect+1: warning: pointer cast from 'pointer to long long' to 'pointer to int' may be troublesome [247] */
	int_ptr = (typeof(int_ptr))llong_ptr;
	/* expect+1: warning: pointer cast from 'pointer to unsigned long long' to 'pointer to int' may be troublesome [247] */
	int_ptr = (typeof(int_ptr))ullong_ptr;
	/* expect+1: warning: pointer cast from 'pointer to float' to 'pointer to int' may be troublesome [247] */
	int_ptr = (typeof(int_ptr))float_ptr;
	/* expect+1: warning: pointer cast from 'pointer to double' to 'pointer to int' may be troublesome [247] */
	int_ptr = (typeof(int_ptr))double_ptr;
	/* expect+1: warning: pointer cast from 'pointer to long double' to 'pointer to int' may be troublesome [247] */
	int_ptr = (typeof(int_ptr))ldouble_ptr;
	/* expect+1: warning: pointer cast from 'pointer to float _Complex' to 'pointer to int' may be troublesome [247] */
	int_ptr = (typeof(int_ptr))fcomplex_ptr;
	/* expect+1: warning: pointer cast from 'pointer to double _Complex' to 'pointer to int' may be troublesome [247] */
	int_ptr = (typeof(int_ptr))dcomplex_ptr;
	/* expect+1: warning: pointer cast from 'pointer to long double _Complex' to 'pointer to int' may be troublesome [247] */
	int_ptr = (typeof(int_ptr))lcomplex_ptr;
	int_ptr = (typeof(int_ptr))void_ptr;
	/* expect+1: warning: pointer cast from 'pointer to struct typedef char_struct' to 'pointer to int' may be troublesome [247] */
	int_ptr = (typeof(int_ptr))char_struct_ptr;
	/* expect+1: warning: pointer cast from 'pointer to struct typedef double_struct' to 'pointer to int' may be troublesome [247] */
	int_ptr = (typeof(int_ptr))double_struct_ptr;
	/* expect+1: warning: pointer cast from 'pointer to union typedef char_union' to 'pointer to int' may be troublesome [247] */
	int_ptr = (typeof(int_ptr))char_union_ptr;
	/* expect+1: warning: pointer cast from 'pointer to union typedef double_union' to 'pointer to int' may be troublesome [247] */
	int_ptr = (typeof(int_ptr))double_union_ptr;
	int_ptr = (typeof(int_ptr))enum_ptr;
	/* expect+1: warning: pointer cast from 'pointer to array[5] of double' to 'pointer to int' may be troublesome [247] */
	int_ptr = (typeof(int_ptr))double_array_ptr;
	/* expect+1: warning: converting 'pointer to function(void) returning void' to 'pointer to int' is questionable [229] */
	int_ptr = (typeof(int_ptr))func_ptr;

	/* expect+1: warning: pointer cast from 'pointer to _Bool' to 'pointer to unsigned int' may be troublesome [247] */
	uint_ptr = (typeof(uint_ptr))bool_ptr;
	uint_ptr = (typeof(uint_ptr))char_ptr;
	/* expect+1: warning: pointer cast from 'pointer to signed char' to 'pointer to unsigned int' may be troublesome [247] */
	uint_ptr = (typeof(uint_ptr))schar_ptr;
	uint_ptr = (typeof(uint_ptr))uchar_ptr;
	/* expect+1: warning: pointer cast from 'pointer to short' to 'pointer to unsigned int' may be troublesome [247] */
	uint_ptr = (typeof(uint_ptr))short_ptr;
	/* expect+1: warning: pointer cast from 'pointer to unsigned short' to 'pointer to unsigned int' may be troublesome [247] */
	uint_ptr = (typeof(uint_ptr))ushort_ptr;
	uint_ptr = (typeof(uint_ptr))int_ptr;
	uint_ptr = (typeof(uint_ptr))uint_ptr;
	/* expect+1: warning: pointer cast from 'pointer to long' to 'pointer to unsigned int' may be troublesome [247] */
	uint_ptr = (typeof(uint_ptr))long_ptr;
	/* expect+1: warning: pointer cast from 'pointer to unsigned long' to 'pointer to unsigned int' may be troublesome [247] */
	uint_ptr = (typeof(uint_ptr))ulong_ptr;
	/* expect+1: warning: pointer cast from 'pointer to long long' to 'pointer to unsigned int' may be troublesome [247] */
	uint_ptr = (typeof(uint_ptr))llong_ptr;
	/* expect+1: warning: pointer cast from 'pointer to unsigned long long' to 'pointer to unsigned int' may be troublesome [247] */
	uint_ptr = (typeof(uint_ptr))ullong_ptr;
	/* expect+1: warning: pointer cast from 'pointer to float' to 'pointer to unsigned int' may be troublesome [247] */
	uint_ptr = (typeof(uint_ptr))float_ptr;
	/* expect+1: warning: pointer cast from 'pointer to double' to 'pointer to unsigned int' may be troublesome [247] */
	uint_ptr = (typeof(uint_ptr))double_ptr;
	/* expect+1: warning: pointer cast from 'pointer to long double' to 'pointer to unsigned int' may be troublesome [247] */
	uint_ptr = (typeof(uint_ptr))ldouble_ptr;
	/* expect+1: warning: pointer cast from 'pointer to float _Complex' to 'pointer to unsigned int' may be troublesome [247] */
	uint_ptr = (typeof(uint_ptr))fcomplex_ptr;
	/* expect+1: warning: pointer cast from 'pointer to double _Complex' to 'pointer to unsigned int' may be troublesome [247] */
	uint_ptr = (typeof(uint_ptr))dcomplex_ptr;
	/* expect+1: warning: pointer cast from 'pointer to long double _Complex' to 'pointer to unsigned int' may be troublesome [247] */
	uint_ptr = (typeof(uint_ptr))lcomplex_ptr;
	uint_ptr = (typeof(uint_ptr))void_ptr;
	/* expect+1: warning: pointer cast from 'pointer to struct typedef char_struct' to 'pointer to unsigned int' may be troublesome [247] */
	uint_ptr = (typeof(uint_ptr))char_struct_ptr;
	/* expect+1: warning: pointer cast from 'pointer to struct typedef double_struct' to 'pointer to unsigned int' may be troublesome [247] */
	uint_ptr = (typeof(uint_ptr))double_struct_ptr;
	/* expect+1: warning: pointer cast from 'pointer to union typedef char_union' to 'pointer to unsigned int' may be troublesome [247] */
	uint_ptr = (typeof(uint_ptr))char_union_ptr;
	/* expect+1: warning: pointer cast from 'pointer to union typedef double_union' to 'pointer to unsigned int' may be troublesome [247] */
	uint_ptr = (typeof(uint_ptr))double_union_ptr;
	uint_ptr = (typeof(uint_ptr))enum_ptr;
	/* expect+1: warning: pointer cast from 'pointer to array[5] of double' to 'pointer to unsigned int' may be troublesome [247] */
	uint_ptr = (typeof(uint_ptr))double_array_ptr;
	/* expect+1: warning: converting 'pointer to function(void) returning void' to 'pointer to unsigned int' is questionable [229] */
	uint_ptr = (typeof(uint_ptr))func_ptr;

	/* expect+1: warning: pointer cast from 'pointer to _Bool' to 'pointer to long' may be troublesome [247] */
	long_ptr = (typeof(long_ptr))bool_ptr;
	long_ptr = (typeof(long_ptr))char_ptr;
	/* expect+1: warning: pointer cast from 'pointer to signed char' to 'pointer to long' may be troublesome [247] */
	long_ptr = (typeof(long_ptr))schar_ptr;
	long_ptr = (typeof(long_ptr))uchar_ptr;
	/* expect+1: warning: pointer cast from 'pointer to short' to 'pointer to long' may be troublesome [247] */
	long_ptr = (typeof(long_ptr))short_ptr;
	/* expect+1: warning: pointer cast from 'pointer to unsigned short' to 'pointer to long' may be troublesome [247] */
	long_ptr = (typeof(long_ptr))ushort_ptr;
	/* expect+1: warning: pointer cast from 'pointer to int' to 'pointer to long' may be troublesome [247] */
	long_ptr = (typeof(long_ptr))int_ptr;
	/* expect+1: warning: pointer cast from 'pointer to unsigned int' to 'pointer to long' may be troublesome [247] */
	long_ptr = (typeof(long_ptr))uint_ptr;
	long_ptr = (typeof(long_ptr))long_ptr;
	long_ptr = (typeof(long_ptr))ulong_ptr;
	long_ptr = (typeof(long_ptr))llong_ptr;
	long_ptr = (typeof(long_ptr))ullong_ptr;
	/* expect+1: warning: pointer cast from 'pointer to float' to 'pointer to long' may be troublesome [247] */
	long_ptr = (typeof(long_ptr))float_ptr;
	/* expect+1: warning: pointer cast from 'pointer to double' to 'pointer to long' may be troublesome [247] */
	long_ptr = (typeof(long_ptr))double_ptr;
	/* expect+1: warning: pointer cast from 'pointer to long double' to 'pointer to long' may be troublesome [247] */
	long_ptr = (typeof(long_ptr))ldouble_ptr;
	/* expect+1: warning: pointer cast from 'pointer to float _Complex' to 'pointer to long' may be troublesome [247] */
	long_ptr = (typeof(long_ptr))fcomplex_ptr;
	/* expect+1: warning: pointer cast from 'pointer to double _Complex' to 'pointer to long' may be troublesome [247] */
	long_ptr = (typeof(long_ptr))dcomplex_ptr;
	/* expect+1: warning: pointer cast from 'pointer to long double _Complex' to 'pointer to long' may be troublesome [247] */
	long_ptr = (typeof(long_ptr))lcomplex_ptr;
	long_ptr = (typeof(long_ptr))void_ptr;
	/* expect+1: warning: pointer cast from 'pointer to struct typedef char_struct' to 'pointer to long' may be troublesome [247] */
	long_ptr = (typeof(long_ptr))char_struct_ptr;
	/* expect+1: warning: pointer cast from 'pointer to struct typedef double_struct' to 'pointer to long' may be troublesome [247] */
	long_ptr = (typeof(long_ptr))double_struct_ptr;
	/* expect+1: warning: pointer cast from 'pointer to union typedef char_union' to 'pointer to long' may be troublesome [247] */
	long_ptr = (typeof(long_ptr))char_union_ptr;
	/* expect+1: warning: pointer cast from 'pointer to union typedef double_union' to 'pointer to long' may be troublesome [247] */
	long_ptr = (typeof(long_ptr))double_union_ptr;
	/* expect+1: warning: pointer cast from 'pointer to enum typedef int_enum' to 'pointer to long' may be troublesome [247] */
	long_ptr = (typeof(long_ptr))enum_ptr;
	/* expect+1: warning: pointer cast from 'pointer to array[5] of double' to 'pointer to long' may be troublesome [247] */
	long_ptr = (typeof(long_ptr))double_array_ptr;
	/* expect+1: warning: converting 'pointer to function(void) returning void' to 'pointer to long' is questionable [229] */
	long_ptr = (typeof(long_ptr))func_ptr;

	/* expect+1: warning: pointer cast from 'pointer to _Bool' to 'pointer to unsigned long' may be troublesome [247] */
	ulong_ptr = (typeof(ulong_ptr))bool_ptr;
	ulong_ptr = (typeof(ulong_ptr))char_ptr;
	/* expect+1: warning: pointer cast from 'pointer to signed char' to 'pointer to unsigned long' may be troublesome [247] */
	ulong_ptr = (typeof(ulong_ptr))schar_ptr;
	ulong_ptr = (typeof(ulong_ptr))uchar_ptr;
	/* expect+1: warning: pointer cast from 'pointer to short' to 'pointer to unsigned long' may be troublesome [247] */
	ulong_ptr = (typeof(ulong_ptr))short_ptr;
	/* expect+1: warning: pointer cast from 'pointer to unsigned short' to 'pointer to unsigned long' may be troublesome [247] */
	ulong_ptr = (typeof(ulong_ptr))ushort_ptr;
	/* expect+1: warning: pointer cast from 'pointer to int' to 'pointer to unsigned long' may be troublesome [247] */
	ulong_ptr = (typeof(ulong_ptr))int_ptr;
	/* expect+1: warning: pointer cast from 'pointer to unsigned int' to 'pointer to unsigned long' may be troublesome [247] */
	ulong_ptr = (typeof(ulong_ptr))uint_ptr;
	ulong_ptr = (typeof(ulong_ptr))long_ptr;
	ulong_ptr = (typeof(ulong_ptr))ulong_ptr;
	ulong_ptr = (typeof(ulong_ptr))llong_ptr;
	ulong_ptr = (typeof(ulong_ptr))ullong_ptr;
	/* expect+1: warning: pointer cast from 'pointer to float' to 'pointer to unsigned long' may be troublesome [247] */
	ulong_ptr = (typeof(ulong_ptr))float_ptr;
	/* expect+1: warning: pointer cast from 'pointer to double' to 'pointer to unsigned long' may be troublesome [247] */
	ulong_ptr = (typeof(ulong_ptr))double_ptr;
	/* expect+1: warning: pointer cast from 'pointer to long double' to 'pointer to unsigned long' may be troublesome [247] */
	ulong_ptr = (typeof(ulong_ptr))ldouble_ptr;
	/* expect+1: warning: pointer cast from 'pointer to float _Complex' to 'pointer to unsigned long' may be troublesome [247] */
	ulong_ptr = (typeof(ulong_ptr))fcomplex_ptr;
	/* expect+1: warning: pointer cast from 'pointer to double _Complex' to 'pointer to unsigned long' may be troublesome [247] */
	ulong_ptr = (typeof(ulong_ptr))dcomplex_ptr;
	/* expect+1: warning: pointer cast from 'pointer to long double _Complex' to 'pointer to unsigned long' may be troublesome [247] */
	ulong_ptr = (typeof(ulong_ptr))lcomplex_ptr;
	ulong_ptr = (typeof(ulong_ptr))void_ptr;
	/* expect+1: warning: pointer cast from 'pointer to struct typedef char_struct' to 'pointer to unsigned long' may be troublesome [247] */
	ulong_ptr = (typeof(ulong_ptr))char_struct_ptr;
	/* expect+1: warning: pointer cast from 'pointer to struct typedef double_struct' to 'pointer to unsigned long' may be troublesome [247] */
	ulong_ptr = (typeof(ulong_ptr))double_struct_ptr;
	/* expect+1: warning: pointer cast from 'pointer to union typedef char_union' to 'pointer to unsigned long' may be troublesome [247] */
	ulong_ptr = (typeof(ulong_ptr))char_union_ptr;
	/* expect+1: warning: pointer cast from 'pointer to union typedef double_union' to 'pointer to unsigned long' may be troublesome [247] */
	ulong_ptr = (typeof(ulong_ptr))double_union_ptr;
	/* expect+1: warning: pointer cast from 'pointer to enum typedef int_enum' to 'pointer to unsigned long' may be troublesome [247] */
	ulong_ptr = (typeof(ulong_ptr))enum_ptr;
	/* expect+1: warning: pointer cast from 'pointer to array[5] of double' to 'pointer to unsigned long' may be troublesome [247] */
	ulong_ptr = (typeof(ulong_ptr))double_array_ptr;
	/* expect+1: warning: converting 'pointer to function(void) returning void' to 'pointer to unsigned long' is questionable [229] */
	ulong_ptr = (typeof(ulong_ptr))func_ptr;

	/* expect+1: warning: pointer cast from 'pointer to _Bool' to 'pointer to long long' may be troublesome [247] */
	llong_ptr = (typeof(llong_ptr))bool_ptr;
	llong_ptr = (typeof(llong_ptr))char_ptr;
	/* expect+1: warning: pointer cast from 'pointer to signed char' to 'pointer to long long' may be troublesome [247] */
	llong_ptr = (typeof(llong_ptr))schar_ptr;
	llong_ptr = (typeof(llong_ptr))uchar_ptr;
	/* expect+1: warning: pointer cast from 'pointer to short' to 'pointer to long long' may be troublesome [247] */
	llong_ptr = (typeof(llong_ptr))short_ptr;
	/* expect+1: warning: pointer cast from 'pointer to unsigned short' to 'pointer to long long' may be troublesome [247] */
	llong_ptr = (typeof(llong_ptr))ushort_ptr;
	/* expect+1: warning: pointer cast from 'pointer to int' to 'pointer to long long' may be troublesome [247] */
	llong_ptr = (typeof(llong_ptr))int_ptr;
	/* expect+1: warning: pointer cast from 'pointer to unsigned int' to 'pointer to long long' may be troublesome [247] */
	llong_ptr = (typeof(llong_ptr))uint_ptr;
	llong_ptr = (typeof(llong_ptr))long_ptr;
	llong_ptr = (typeof(llong_ptr))ulong_ptr;
	llong_ptr = (typeof(llong_ptr))llong_ptr;
	llong_ptr = (typeof(llong_ptr))ullong_ptr;
	/* expect+1: warning: pointer cast from 'pointer to float' to 'pointer to long long' may be troublesome [247] */
	llong_ptr = (typeof(llong_ptr))float_ptr;
	/* expect+1: warning: pointer cast from 'pointer to double' to 'pointer to long long' may be troublesome [247] */
	llong_ptr = (typeof(llong_ptr))double_ptr;
	/* expect+1: warning: pointer cast from 'pointer to long double' to 'pointer to long long' may be troublesome [247] */
	llong_ptr = (typeof(llong_ptr))ldouble_ptr;
	/* expect+1: warning: pointer cast from 'pointer to float _Complex' to 'pointer to long long' may be troublesome [247] */
	llong_ptr = (typeof(llong_ptr))fcomplex_ptr;
	/* expect+1: warning: pointer cast from 'pointer to double _Complex' to 'pointer to long long' may be troublesome [247] */
	llong_ptr = (typeof(llong_ptr))dcomplex_ptr;
	/* expect+1: warning: pointer cast from 'pointer to long double _Complex' to 'pointer to long long' may be troublesome [247] */
	llong_ptr = (typeof(llong_ptr))lcomplex_ptr;
	llong_ptr = (typeof(llong_ptr))void_ptr;
	/* expect+1: warning: pointer cast from 'pointer to struct typedef char_struct' to 'pointer to long long' may be troublesome [247] */
	llong_ptr = (typeof(llong_ptr))char_struct_ptr;
	/* expect+1: warning: pointer cast from 'pointer to struct typedef double_struct' to 'pointer to long long' may be troublesome [247] */
	llong_ptr = (typeof(llong_ptr))double_struct_ptr;
	/* expect+1: warning: pointer cast from 'pointer to union typedef char_union' to 'pointer to long long' may be troublesome [247] */
	llong_ptr = (typeof(llong_ptr))char_union_ptr;
	/* expect+1: warning: pointer cast from 'pointer to union typedef double_union' to 'pointer to long long' may be troublesome [247] */
	llong_ptr = (typeof(llong_ptr))double_union_ptr;
	/* expect+1: warning: pointer cast from 'pointer to enum typedef int_enum' to 'pointer to long long' may be troublesome [247] */
	llong_ptr = (typeof(llong_ptr))enum_ptr;
	/* expect+1: warning: pointer cast from 'pointer to array[5] of double' to 'pointer to long long' may be troublesome [247] */
	llong_ptr = (typeof(llong_ptr))double_array_ptr;
	/* expect+1: warning: converting 'pointer to function(void) returning void' to 'pointer to long long' is questionable [229] */
	llong_ptr = (typeof(llong_ptr))func_ptr;

	/* expect+1: warning: pointer cast from 'pointer to _Bool' to 'pointer to unsigned long long' may be troublesome [247] */
	ullong_ptr = (typeof(ullong_ptr))bool_ptr;
	ullong_ptr = (typeof(ullong_ptr))char_ptr;
	/* expect+1: warning: pointer cast from 'pointer to signed char' to 'pointer to unsigned long long' may be troublesome [247] */
	ullong_ptr = (typeof(ullong_ptr))schar_ptr;
	ullong_ptr = (typeof(ullong_ptr))uchar_ptr;
	/* expect+1: warning: pointer cast from 'pointer to short' to 'pointer to unsigned long long' may be troublesome [247] */
	ullong_ptr = (typeof(ullong_ptr))short_ptr;
	/* expect+1: warning: pointer cast from 'pointer to unsigned short' to 'pointer to unsigned long long' may be troublesome [247] */
	ullong_ptr = (typeof(ullong_ptr))ushort_ptr;
	/* expect+1: warning: pointer cast from 'pointer to int' to 'pointer to unsigned long long' may be troublesome [247] */
	ullong_ptr = (typeof(ullong_ptr))int_ptr;
	/* expect+1: warning: pointer cast from 'pointer to unsigned int' to 'pointer to unsigned long long' may be troublesome [247] */
	ullong_ptr = (typeof(ullong_ptr))uint_ptr;
	ullong_ptr = (typeof(ullong_ptr))long_ptr;
	ullong_ptr = (typeof(ullong_ptr))ulong_ptr;
	ullong_ptr = (typeof(ullong_ptr))llong_ptr;
	ullong_ptr = (typeof(ullong_ptr))ullong_ptr;
	/* expect+1: warning: pointer cast from 'pointer to float' to 'pointer to unsigned long long' may be troublesome [247] */
	ullong_ptr = (typeof(ullong_ptr))float_ptr;
	/* expect+1: warning: pointer cast from 'pointer to double' to 'pointer to unsigned long long' may be troublesome [247] */
	ullong_ptr = (typeof(ullong_ptr))double_ptr;
	/* expect+1: warning: pointer cast from 'pointer to long double' to 'pointer to unsigned long long' may be troublesome [247] */
	ullong_ptr = (typeof(ullong_ptr))ldouble_ptr;
	/* expect+1: warning: pointer cast from 'pointer to float _Complex' to 'pointer to unsigned long long' may be troublesome [247] */
	ullong_ptr = (typeof(ullong_ptr))fcomplex_ptr;
	/* expect+1: warning: pointer cast from 'pointer to double _Complex' to 'pointer to unsigned long long' may be troublesome [247] */
	ullong_ptr = (typeof(ullong_ptr))dcomplex_ptr;
	/* expect+1: warning: pointer cast from 'pointer to long double _Complex' to 'pointer to unsigned long long' may be troublesome [247] */
	ullong_ptr = (typeof(ullong_ptr))lcomplex_ptr;
	ullong_ptr = (typeof(ullong_ptr))void_ptr;
	/* expect+1: warning: pointer cast from 'pointer to struct typedef char_struct' to 'pointer to unsigned long long' may be troublesome [247] */
	ullong_ptr = (typeof(ullong_ptr))char_struct_ptr;
	/* expect+1: warning: pointer cast from 'pointer to struct typedef double_struct' to 'pointer to unsigned long long' may be troublesome [247] */
	ullong_ptr = (typeof(ullong_ptr))double_struct_ptr;
	/* expect+1: warning: pointer cast from 'pointer to union typedef char_union' to 'pointer to unsigned long long' may be troublesome [247] */
	ullong_ptr = (typeof(ullong_ptr))char_union_ptr;
	/* expect+1: warning: pointer cast from 'pointer to union typedef double_union' to 'pointer to unsigned long long' may be troublesome [247] */
	ullong_ptr = (typeof(ullong_ptr))double_union_ptr;
	/* expect+1: warning: pointer cast from 'pointer to enum typedef int_enum' to 'pointer to unsigned long long' may be troublesome [247] */
	ullong_ptr = (typeof(ullong_ptr))enum_ptr;
	/* expect+1: warning: pointer cast from 'pointer to array[5] of double' to 'pointer to unsigned long long' may be troublesome [247] */
	ullong_ptr = (typeof(ullong_ptr))double_array_ptr;
	/* expect+1: warning: converting 'pointer to function(void) returning void' to 'pointer to unsigned long long' is questionable [229] */
	ullong_ptr = (typeof(ullong_ptr))func_ptr;

	/* expect+1: warning: pointer cast from 'pointer to _Bool' to 'pointer to float' may be troublesome [247] */
	float_ptr = (typeof(float_ptr))bool_ptr;
	float_ptr = (typeof(float_ptr))char_ptr;
	/* expect+1: warning: pointer cast from 'pointer to signed char' to 'pointer to float' may be troublesome [247] */
	float_ptr = (typeof(float_ptr))schar_ptr;
	float_ptr = (typeof(float_ptr))uchar_ptr;
	/* expect+1: warning: pointer cast from 'pointer to short' to 'pointer to float' may be troublesome [247] */
	float_ptr = (typeof(float_ptr))short_ptr;
	/* expect+1: warning: pointer cast from 'pointer to unsigned short' to 'pointer to float' may be troublesome [247] */
	float_ptr = (typeof(float_ptr))ushort_ptr;
	/* expect+1: warning: pointer cast from 'pointer to int' to 'pointer to float' may be troublesome [247] */
	float_ptr = (typeof(float_ptr))int_ptr;
	/* expect+1: warning: pointer cast from 'pointer to unsigned int' to 'pointer to float' may be troublesome [247] */
	float_ptr = (typeof(float_ptr))uint_ptr;
	/* expect+1: warning: pointer cast from 'pointer to long' to 'pointer to float' may be troublesome [247] */
	float_ptr = (typeof(float_ptr))long_ptr;
	/* expect+1: warning: pointer cast from 'pointer to unsigned long' to 'pointer to float' may be troublesome [247] */
	float_ptr = (typeof(float_ptr))ulong_ptr;
	/* expect+1: warning: pointer cast from 'pointer to long long' to 'pointer to float' may be troublesome [247] */
	float_ptr = (typeof(float_ptr))llong_ptr;
	/* expect+1: warning: pointer cast from 'pointer to unsigned long long' to 'pointer to float' may be troublesome [247] */
	float_ptr = (typeof(float_ptr))ullong_ptr;
	float_ptr = (typeof(float_ptr))float_ptr;
	/* expect+1: warning: pointer cast from 'pointer to double' to 'pointer to float' may be troublesome [247] */
	float_ptr = (typeof(float_ptr))double_ptr;
	/* expect+1: warning: pointer cast from 'pointer to long double' to 'pointer to float' may be troublesome [247] */
	float_ptr = (typeof(float_ptr))ldouble_ptr;
	/* expect+1: warning: pointer cast from 'pointer to float _Complex' to 'pointer to float' may be troublesome [247] */
	float_ptr = (typeof(float_ptr))fcomplex_ptr;
	/* expect+1: warning: pointer cast from 'pointer to double _Complex' to 'pointer to float' may be troublesome [247] */
	float_ptr = (typeof(float_ptr))dcomplex_ptr;
	/* expect+1: warning: pointer cast from 'pointer to long double _Complex' to 'pointer to float' may be troublesome [247] */
	float_ptr = (typeof(float_ptr))lcomplex_ptr;
	float_ptr = (typeof(float_ptr))void_ptr;
	/* expect+1: warning: pointer cast from 'pointer to struct typedef char_struct' to 'pointer to float' may be troublesome [247] */
	float_ptr = (typeof(float_ptr))char_struct_ptr;
	/* expect+1: warning: pointer cast from 'pointer to struct typedef double_struct' to 'pointer to float' may be troublesome [247] */
	float_ptr = (typeof(float_ptr))double_struct_ptr;
	/* expect+1: warning: pointer cast from 'pointer to union typedef char_union' to 'pointer to float' may be troublesome [247] */
	float_ptr = (typeof(float_ptr))char_union_ptr;
	/* expect+1: warning: pointer cast from 'pointer to union typedef double_union' to 'pointer to float' may be troublesome [247] */
	float_ptr = (typeof(float_ptr))double_union_ptr;
	/* expect+1: warning: pointer cast from 'pointer to enum typedef int_enum' to 'pointer to float' may be troublesome [247] */
	float_ptr = (typeof(float_ptr))enum_ptr;
	/* expect+1: warning: pointer cast from 'pointer to array[5] of double' to 'pointer to float' may be troublesome [247] */
	float_ptr = (typeof(float_ptr))double_array_ptr;
	/* expect+1: warning: converting 'pointer to function(void) returning void' to 'pointer to float' is questionable [229] */
	float_ptr = (typeof(float_ptr))func_ptr;

	/* expect+1: warning: pointer cast from 'pointer to _Bool' to 'pointer to double' may be troublesome [247] */
	double_ptr = (typeof(double_ptr))bool_ptr;
	double_ptr = (typeof(double_ptr))char_ptr;
	/* expect+1: warning: pointer cast from 'pointer to signed char' to 'pointer to double' may be troublesome [247] */
	double_ptr = (typeof(double_ptr))schar_ptr;
	double_ptr = (typeof(double_ptr))uchar_ptr;
	/* expect+1: warning: pointer cast from 'pointer to short' to 'pointer to double' may be troublesome [247] */
	double_ptr = (typeof(double_ptr))short_ptr;
	/* expect+1: warning: pointer cast from 'pointer to unsigned short' to 'pointer to double' may be troublesome [247] */
	double_ptr = (typeof(double_ptr))ushort_ptr;
	/* expect+1: warning: pointer cast from 'pointer to int' to 'pointer to double' may be troublesome [247] */
	double_ptr = (typeof(double_ptr))int_ptr;
	/* expect+1: warning: pointer cast from 'pointer to unsigned int' to 'pointer to double' may be troublesome [247] */
	double_ptr = (typeof(double_ptr))uint_ptr;
	/* expect+1: warning: pointer cast from 'pointer to long' to 'pointer to double' may be troublesome [247] */
	double_ptr = (typeof(double_ptr))long_ptr;
	/* expect+1: warning: pointer cast from 'pointer to unsigned long' to 'pointer to double' may be troublesome [247] */
	double_ptr = (typeof(double_ptr))ulong_ptr;
	/* expect+1: warning: pointer cast from 'pointer to long long' to 'pointer to double' may be troublesome [247] */
	double_ptr = (typeof(double_ptr))llong_ptr;
	/* expect+1: warning: pointer cast from 'pointer to unsigned long long' to 'pointer to double' may be troublesome [247] */
	double_ptr = (typeof(double_ptr))ullong_ptr;
	/* expect+1: warning: pointer cast from 'pointer to float' to 'pointer to double' may be troublesome [247] */
	double_ptr = (typeof(double_ptr))float_ptr;
	double_ptr = (typeof(double_ptr))double_ptr;
	/* expect+1: warning: pointer cast from 'pointer to long double' to 'pointer to double' may be troublesome [247] */
	double_ptr = (typeof(double_ptr))ldouble_ptr;
	/* expect+1: warning: pointer cast from 'pointer to float _Complex' to 'pointer to double' may be troublesome [247] */
	double_ptr = (typeof(double_ptr))fcomplex_ptr;
	/* expect+1: warning: pointer cast from 'pointer to double _Complex' to 'pointer to double' may be troublesome [247] */
	double_ptr = (typeof(double_ptr))dcomplex_ptr;
	/* expect+1: warning: pointer cast from 'pointer to long double _Complex' to 'pointer to double' may be troublesome [247] */
	double_ptr = (typeof(double_ptr))lcomplex_ptr;
	double_ptr = (typeof(double_ptr))void_ptr;
	/* expect+1: warning: pointer cast from 'pointer to struct typedef char_struct' to 'pointer to double' may be troublesome [247] */
	double_ptr = (typeof(double_ptr))char_struct_ptr;
	/* expect+1: warning: pointer cast from 'pointer to struct typedef double_struct' to 'pointer to double' may be troublesome [247] */
	double_ptr = (typeof(double_ptr))double_struct_ptr;
	/* expect+1: warning: pointer cast from 'pointer to union typedef char_union' to 'pointer to double' may be troublesome [247] */
	double_ptr = (typeof(double_ptr))char_union_ptr;
	double_ptr = (typeof(double_ptr))double_union_ptr;
	/* expect+1: warning: pointer cast from 'pointer to enum typedef int_enum' to 'pointer to double' may be troublesome [247] */
	double_ptr = (typeof(double_ptr))enum_ptr;
	double_ptr = (typeof(double_ptr))double_array_ptr;
	/* expect+1: warning: converting 'pointer to function(void) returning void' to 'pointer to double' is questionable [229] */
	double_ptr = (typeof(double_ptr))func_ptr;

	/* expect+1: warning: pointer cast from 'pointer to _Bool' to 'pointer to long double' may be troublesome [247] */
	ldouble_ptr = (typeof(ldouble_ptr))bool_ptr;
	ldouble_ptr = (typeof(ldouble_ptr))char_ptr;
	/* expect+1: warning: pointer cast from 'pointer to signed char' to 'pointer to long double' may be troublesome [247] */
	ldouble_ptr = (typeof(ldouble_ptr))schar_ptr;
	ldouble_ptr = (typeof(ldouble_ptr))uchar_ptr;
	/* expect+1: warning: pointer cast from 'pointer to short' to 'pointer to long double' may be troublesome [247] */
	ldouble_ptr = (typeof(ldouble_ptr))short_ptr;
	/* expect+1: warning: pointer cast from 'pointer to unsigned short' to 'pointer to long double' may be troublesome [247] */
	ldouble_ptr = (typeof(ldouble_ptr))ushort_ptr;
	/* expect+1: warning: pointer cast from 'pointer to int' to 'pointer to long double' may be troublesome [247] */
	ldouble_ptr = (typeof(ldouble_ptr))int_ptr;
	/* expect+1: warning: pointer cast from 'pointer to unsigned int' to 'pointer to long double' may be troublesome [247] */
	ldouble_ptr = (typeof(ldouble_ptr))uint_ptr;
	/* expect+1: warning: pointer cast from 'pointer to long' to 'pointer to long double' may be troublesome [247] */
	ldouble_ptr = (typeof(ldouble_ptr))long_ptr;
	/* expect+1: warning: pointer cast from 'pointer to unsigned long' to 'pointer to long double' may be troublesome [247] */
	ldouble_ptr = (typeof(ldouble_ptr))ulong_ptr;
	/* expect+1: warning: pointer cast from 'pointer to long long' to 'pointer to long double' may be troublesome [247] */
	ldouble_ptr = (typeof(ldouble_ptr))llong_ptr;
	/* expect+1: warning: pointer cast from 'pointer to unsigned long long' to 'pointer to long double' may be troublesome [247] */
	ldouble_ptr = (typeof(ldouble_ptr))ullong_ptr;
	/* expect+1: warning: pointer cast from 'pointer to float' to 'pointer to long double' may be troublesome [247] */
	ldouble_ptr = (typeof(ldouble_ptr))float_ptr;
	/* expect+1: warning: pointer cast from 'pointer to double' to 'pointer to long double' may be troublesome [247] */
	ldouble_ptr = (typeof(ldouble_ptr))double_ptr;
	ldouble_ptr = (typeof(ldouble_ptr))ldouble_ptr;
	/* expect+1: warning: pointer cast from 'pointer to float _Complex' to 'pointer to long double' may be troublesome [247] */
	ldouble_ptr = (typeof(ldouble_ptr))fcomplex_ptr;
	/* expect+1: warning: pointer cast from 'pointer to double _Complex' to 'pointer to long double' may be troublesome [247] */
	ldouble_ptr = (typeof(ldouble_ptr))dcomplex_ptr;
	/* expect+1: warning: pointer cast from 'pointer to long double _Complex' to 'pointer to long double' may be troublesome [247] */
	ldouble_ptr = (typeof(ldouble_ptr))lcomplex_ptr;
	ldouble_ptr = (typeof(ldouble_ptr))void_ptr;
	/* expect+1: warning: pointer cast from 'pointer to struct typedef char_struct' to 'pointer to long double' may be troublesome [247] */
	ldouble_ptr = (typeof(ldouble_ptr))char_struct_ptr;
	/* expect+1: warning: pointer cast from 'pointer to struct typedef double_struct' to 'pointer to long double' may be troublesome [247] */
	ldouble_ptr = (typeof(ldouble_ptr))double_struct_ptr;
	/* expect+1: warning: pointer cast from 'pointer to union typedef char_union' to 'pointer to long double' may be troublesome [247] */
	ldouble_ptr = (typeof(ldouble_ptr))char_union_ptr;
	/* expect+1: warning: pointer cast from 'pointer to union typedef double_union' to 'pointer to long double' may be troublesome [247] */
	ldouble_ptr = (typeof(ldouble_ptr))double_union_ptr;
	/* expect+1: warning: pointer cast from 'pointer to enum typedef int_enum' to 'pointer to long double' may be troublesome [247] */
	ldouble_ptr = (typeof(ldouble_ptr))enum_ptr;
	/* expect+1: warning: pointer cast from 'pointer to array[5] of double' to 'pointer to long double' may be troublesome [247] */
	ldouble_ptr = (typeof(ldouble_ptr))double_array_ptr;
	/* expect+1: warning: converting 'pointer to function(void) returning void' to 'pointer to long double' is questionable [229] */
	ldouble_ptr = (typeof(ldouble_ptr))func_ptr;

	/* expect+1: warning: pointer cast from 'pointer to _Bool' to 'pointer to float _Complex' may be troublesome [247] */
	fcomplex_ptr = (typeof(fcomplex_ptr))bool_ptr;
	fcomplex_ptr = (typeof(fcomplex_ptr))char_ptr;
	/* expect+1: warning: pointer cast from 'pointer to signed char' to 'pointer to float _Complex' may be troublesome [247] */
	fcomplex_ptr = (typeof(fcomplex_ptr))schar_ptr;
	fcomplex_ptr = (typeof(fcomplex_ptr))uchar_ptr;
	/* expect+1: warning: pointer cast from 'pointer to short' to 'pointer to float _Complex' may be troublesome [247] */
	fcomplex_ptr = (typeof(fcomplex_ptr))short_ptr;
	/* expect+1: warning: pointer cast from 'pointer to unsigned short' to 'pointer to float _Complex' may be troublesome [247] */
	fcomplex_ptr = (typeof(fcomplex_ptr))ushort_ptr;
	/* expect+1: warning: pointer cast from 'pointer to int' to 'pointer to float _Complex' may be troublesome [247] */
	fcomplex_ptr = (typeof(fcomplex_ptr))int_ptr;
	/* expect+1: warning: pointer cast from 'pointer to unsigned int' to 'pointer to float _Complex' may be troublesome [247] */
	fcomplex_ptr = (typeof(fcomplex_ptr))uint_ptr;
	/* expect+1: warning: pointer cast from 'pointer to long' to 'pointer to float _Complex' may be troublesome [247] */
	fcomplex_ptr = (typeof(fcomplex_ptr))long_ptr;
	/* expect+1: warning: pointer cast from 'pointer to unsigned long' to 'pointer to float _Complex' may be troublesome [247] */
	fcomplex_ptr = (typeof(fcomplex_ptr))ulong_ptr;
	/* expect+1: warning: pointer cast from 'pointer to long long' to 'pointer to float _Complex' may be troublesome [247] */
	fcomplex_ptr = (typeof(fcomplex_ptr))llong_ptr;
	/* expect+1: warning: pointer cast from 'pointer to unsigned long long' to 'pointer to float _Complex' may be troublesome [247] */
	fcomplex_ptr = (typeof(fcomplex_ptr))ullong_ptr;
	/* expect+1: warning: pointer cast from 'pointer to float' to 'pointer to float _Complex' may be troublesome [247] */
	fcomplex_ptr = (typeof(fcomplex_ptr))float_ptr;
	/* expect+1: warning: pointer cast from 'pointer to double' to 'pointer to float _Complex' may be troublesome [247] */
	fcomplex_ptr = (typeof(fcomplex_ptr))double_ptr;
	/* expect+1: warning: pointer cast from 'pointer to long double' to 'pointer to float _Complex' may be troublesome [247] */
	fcomplex_ptr = (typeof(fcomplex_ptr))ldouble_ptr;
	fcomplex_ptr = (typeof(fcomplex_ptr))fcomplex_ptr;
	/* expect+1: warning: pointer cast from 'pointer to double _Complex' to 'pointer to float _Complex' may be troublesome [247] */
	fcomplex_ptr = (typeof(fcomplex_ptr))dcomplex_ptr;
	/* expect+1: warning: pointer cast from 'pointer to long double _Complex' to 'pointer to float _Complex' may be troublesome [247] */
	fcomplex_ptr = (typeof(fcomplex_ptr))lcomplex_ptr;
	fcomplex_ptr = (typeof(fcomplex_ptr))void_ptr;
	/* expect+1: warning: pointer cast from 'pointer to struct typedef char_struct' to 'pointer to float _Complex' may be troublesome [247] */
	fcomplex_ptr = (typeof(fcomplex_ptr))char_struct_ptr;
	/* expect+1: warning: pointer cast from 'pointer to struct typedef double_struct' to 'pointer to float _Complex' may be troublesome [247] */
	fcomplex_ptr = (typeof(fcomplex_ptr))double_struct_ptr;
	/* expect+1: warning: pointer cast from 'pointer to union typedef char_union' to 'pointer to float _Complex' may be troublesome [247] */
	fcomplex_ptr = (typeof(fcomplex_ptr))char_union_ptr;
	/* expect+1: warning: pointer cast from 'pointer to union typedef double_union' to 'pointer to float _Complex' may be troublesome [247] */
	fcomplex_ptr = (typeof(fcomplex_ptr))double_union_ptr;
	/* expect+1: warning: pointer cast from 'pointer to enum typedef int_enum' to 'pointer to float _Complex' may be troublesome [247] */
	fcomplex_ptr = (typeof(fcomplex_ptr))enum_ptr;
	/* expect+1: warning: pointer cast from 'pointer to array[5] of double' to 'pointer to float _Complex' may be troublesome [247] */
	fcomplex_ptr = (typeof(fcomplex_ptr))double_array_ptr;
	/* expect+1: warning: converting 'pointer to function(void) returning void' to 'pointer to float _Complex' is questionable [229] */
	fcomplex_ptr = (typeof(fcomplex_ptr))func_ptr;

	/* expect+1: warning: pointer cast from 'pointer to _Bool' to 'pointer to double _Complex' may be troublesome [247] */
	dcomplex_ptr = (typeof(dcomplex_ptr))bool_ptr;
	dcomplex_ptr = (typeof(dcomplex_ptr))char_ptr;
	/* expect+1: warning: pointer cast from 'pointer to signed char' to 'pointer to double _Complex' may be troublesome [247] */
	dcomplex_ptr = (typeof(dcomplex_ptr))schar_ptr;
	dcomplex_ptr = (typeof(dcomplex_ptr))uchar_ptr;
	/* expect+1: warning: pointer cast from 'pointer to short' to 'pointer to double _Complex' may be troublesome [247] */
	dcomplex_ptr = (typeof(dcomplex_ptr))short_ptr;
	/* expect+1: warning: pointer cast from 'pointer to unsigned short' to 'pointer to double _Complex' may be troublesome [247] */
	dcomplex_ptr = (typeof(dcomplex_ptr))ushort_ptr;
	/* expect+1: warning: pointer cast from 'pointer to int' to 'pointer to double _Complex' may be troublesome [247] */
	dcomplex_ptr = (typeof(dcomplex_ptr))int_ptr;
	/* expect+1: warning: pointer cast from 'pointer to unsigned int' to 'pointer to double _Complex' may be troublesome [247] */
	dcomplex_ptr = (typeof(dcomplex_ptr))uint_ptr;
	/* expect+1: warning: pointer cast from 'pointer to long' to 'pointer to double _Complex' may be troublesome [247] */
	dcomplex_ptr = (typeof(dcomplex_ptr))long_ptr;
	/* expect+1: warning: pointer cast from 'pointer to unsigned long' to 'pointer to double _Complex' may be troublesome [247] */
	dcomplex_ptr = (typeof(dcomplex_ptr))ulong_ptr;
	/* expect+1: warning: pointer cast from 'pointer to long long' to 'pointer to double _Complex' may be troublesome [247] */
	dcomplex_ptr = (typeof(dcomplex_ptr))llong_ptr;
	/* expect+1: warning: pointer cast from 'pointer to unsigned long long' to 'pointer to double _Complex' may be troublesome [247] */
	dcomplex_ptr = (typeof(dcomplex_ptr))ullong_ptr;
	/* expect+1: warning: pointer cast from 'pointer to float' to 'pointer to double _Complex' may be troublesome [247] */
	dcomplex_ptr = (typeof(dcomplex_ptr))float_ptr;
	/* expect+1: warning: pointer cast from 'pointer to double' to 'pointer to double _Complex' may be troublesome [247] */
	dcomplex_ptr = (typeof(dcomplex_ptr))double_ptr;
	/* expect+1: warning: pointer cast from 'pointer to long double' to 'pointer to double _Complex' may be troublesome [247] */
	dcomplex_ptr = (typeof(dcomplex_ptr))ldouble_ptr;
	/* expect+1: warning: pointer cast from 'pointer to float _Complex' to 'pointer to double _Complex' may be troublesome [247] */
	dcomplex_ptr = (typeof(dcomplex_ptr))fcomplex_ptr;
	dcomplex_ptr = (typeof(dcomplex_ptr))dcomplex_ptr;
	/* expect+1: warning: pointer cast from 'pointer to long double _Complex' to 'pointer to double _Complex' may be troublesome [247] */
	dcomplex_ptr = (typeof(dcomplex_ptr))lcomplex_ptr;
	dcomplex_ptr = (typeof(dcomplex_ptr))void_ptr;
	/* expect+1: warning: pointer cast from 'pointer to struct typedef char_struct' to 'pointer to double _Complex' may be troublesome [247] */
	dcomplex_ptr = (typeof(dcomplex_ptr))char_struct_ptr;
	/* expect+1: warning: pointer cast from 'pointer to struct typedef double_struct' to 'pointer to double _Complex' may be troublesome [247] */
	dcomplex_ptr = (typeof(dcomplex_ptr))double_struct_ptr;
	/* expect+1: warning: pointer cast from 'pointer to union typedef char_union' to 'pointer to double _Complex' may be troublesome [247] */
	dcomplex_ptr = (typeof(dcomplex_ptr))char_union_ptr;
	/* expect+1: warning: pointer cast from 'pointer to union typedef double_union' to 'pointer to double _Complex' may be troublesome [247] */
	dcomplex_ptr = (typeof(dcomplex_ptr))double_union_ptr;
	/* expect+1: warning: pointer cast from 'pointer to enum typedef int_enum' to 'pointer to double _Complex' may be troublesome [247] */
	dcomplex_ptr = (typeof(dcomplex_ptr))enum_ptr;
	/* expect+1: warning: pointer cast from 'pointer to array[5] of double' to 'pointer to double _Complex' may be troublesome [247] */
	dcomplex_ptr = (typeof(dcomplex_ptr))double_array_ptr;
	/* expect+1: warning: converting 'pointer to function(void) returning void' to 'pointer to double _Complex' is questionable [229] */
	dcomplex_ptr = (typeof(dcomplex_ptr))func_ptr;

	/* expect+1: warning: pointer cast from 'pointer to _Bool' to 'pointer to long double _Complex' may be troublesome [247] */
	lcomplex_ptr = (typeof(lcomplex_ptr))bool_ptr;
	lcomplex_ptr = (typeof(lcomplex_ptr))char_ptr;
	/* expect+1: warning: pointer cast from 'pointer to signed char' to 'pointer to long double _Complex' may be troublesome [247] */
	lcomplex_ptr = (typeof(lcomplex_ptr))schar_ptr;
	lcomplex_ptr = (typeof(lcomplex_ptr))uchar_ptr;
	/* expect+1: warning: pointer cast from 'pointer to short' to 'pointer to long double _Complex' may be troublesome [247] */
	lcomplex_ptr = (typeof(lcomplex_ptr))short_ptr;
	/* expect+1: warning: pointer cast from 'pointer to unsigned short' to 'pointer to long double _Complex' may be troublesome [247] */
	lcomplex_ptr = (typeof(lcomplex_ptr))ushort_ptr;
	/* expect+1: warning: pointer cast from 'pointer to int' to 'pointer to long double _Complex' may be troublesome [247] */
	lcomplex_ptr = (typeof(lcomplex_ptr))int_ptr;
	/* expect+1: warning: pointer cast from 'pointer to unsigned int' to 'pointer to long double _Complex' may be troublesome [247] */
	lcomplex_ptr = (typeof(lcomplex_ptr))uint_ptr;
	/* expect+1: warning: pointer cast from 'pointer to long' to 'pointer to long double _Complex' may be troublesome [247] */
	lcomplex_ptr = (typeof(lcomplex_ptr))long_ptr;
	/* expect+1: warning: pointer cast from 'pointer to unsigned long' to 'pointer to long double _Complex' may be troublesome [247] */
	lcomplex_ptr = (typeof(lcomplex_ptr))ulong_ptr;
	/* expect+1: warning: pointer cast from 'pointer to long long' to 'pointer to long double _Complex' may be troublesome [247] */
	lcomplex_ptr = (typeof(lcomplex_ptr))llong_ptr;
	/* expect+1: warning: pointer cast from 'pointer to unsigned long long' to 'pointer to long double _Complex' may be troublesome [247] */
	lcomplex_ptr = (typeof(lcomplex_ptr))ullong_ptr;
	/* expect+1: warning: pointer cast from 'pointer to float' to 'pointer to long double _Complex' may be troublesome [247] */
	lcomplex_ptr = (typeof(lcomplex_ptr))float_ptr;
	/* expect+1: warning: pointer cast from 'pointer to double' to 'pointer to long double _Complex' may be troublesome [247] */
	lcomplex_ptr = (typeof(lcomplex_ptr))double_ptr;
	/* expect+1: warning: pointer cast from 'pointer to long double' to 'pointer to long double _Complex' may be troublesome [247] */
	lcomplex_ptr = (typeof(lcomplex_ptr))ldouble_ptr;
	/* expect+1: warning: pointer cast from 'pointer to float _Complex' to 'pointer to long double _Complex' may be troublesome [247] */
	lcomplex_ptr = (typeof(lcomplex_ptr))fcomplex_ptr;
	/* expect+1: warning: pointer cast from 'pointer to double _Complex' to 'pointer to long double _Complex' may be troublesome [247] */
	lcomplex_ptr = (typeof(lcomplex_ptr))dcomplex_ptr;
	lcomplex_ptr = (typeof(lcomplex_ptr))lcomplex_ptr;
	lcomplex_ptr = (typeof(lcomplex_ptr))void_ptr;
	/* expect+1: warning: pointer cast from 'pointer to struct typedef char_struct' to 'pointer to long double _Complex' may be troublesome [247] */
	lcomplex_ptr = (typeof(lcomplex_ptr))char_struct_ptr;
	/* expect+1: warning: pointer cast from 'pointer to struct typedef double_struct' to 'pointer to long double _Complex' may be troublesome [247] */
	lcomplex_ptr = (typeof(lcomplex_ptr))double_struct_ptr;
	/* expect+1: warning: pointer cast from 'pointer to union typedef char_union' to 'pointer to long double _Complex' may be troublesome [247] */
	lcomplex_ptr = (typeof(lcomplex_ptr))char_union_ptr;
	/* expect+1: warning: pointer cast from 'pointer to union typedef double_union' to 'pointer to long double _Complex' may be troublesome [247] */
	lcomplex_ptr = (typeof(lcomplex_ptr))double_union_ptr;
	/* expect+1: warning: pointer cast from 'pointer to enum typedef int_enum' to 'pointer to long double _Complex' may be troublesome [247] */
	lcomplex_ptr = (typeof(lcomplex_ptr))enum_ptr;
	/* expect+1: warning: pointer cast from 'pointer to array[5] of double' to 'pointer to long double _Complex' may be troublesome [247] */
	lcomplex_ptr = (typeof(lcomplex_ptr))double_array_ptr;
	/* expect+1: warning: converting 'pointer to function(void) returning void' to 'pointer to long double _Complex' is questionable [229] */
	lcomplex_ptr = (typeof(lcomplex_ptr))func_ptr;

	void_ptr = (typeof(void_ptr))bool_ptr;
	void_ptr = (typeof(void_ptr))char_ptr;
	void_ptr = (typeof(void_ptr))schar_ptr;
	void_ptr = (typeof(void_ptr))uchar_ptr;
	void_ptr = (typeof(void_ptr))short_ptr;
	void_ptr = (typeof(void_ptr))ushort_ptr;
	void_ptr = (typeof(void_ptr))int_ptr;
	void_ptr = (typeof(void_ptr))uint_ptr;
	void_ptr = (typeof(void_ptr))long_ptr;
	void_ptr = (typeof(void_ptr))ulong_ptr;
	void_ptr = (typeof(void_ptr))llong_ptr;
	void_ptr = (typeof(void_ptr))ullong_ptr;
	void_ptr = (typeof(void_ptr))float_ptr;
	void_ptr = (typeof(void_ptr))double_ptr;
	void_ptr = (typeof(void_ptr))ldouble_ptr;
	void_ptr = (typeof(void_ptr))fcomplex_ptr;
	void_ptr = (typeof(void_ptr))dcomplex_ptr;
	void_ptr = (typeof(void_ptr))lcomplex_ptr;
	void_ptr = (typeof(void_ptr))void_ptr;
	void_ptr = (typeof(void_ptr))char_struct_ptr;
	void_ptr = (typeof(void_ptr))double_struct_ptr;
	void_ptr = (typeof(void_ptr))char_union_ptr;
	void_ptr = (typeof(void_ptr))double_union_ptr;
	void_ptr = (typeof(void_ptr))enum_ptr;
	void_ptr = (typeof(void_ptr))double_array_ptr;
	void_ptr = (typeof(void_ptr))func_ptr;

	/* expect+1: warning: pointer cast from 'pointer to _Bool' to 'pointer to struct typedef char_struct' may be troublesome [247] */
	char_struct_ptr = (typeof(char_struct_ptr))bool_ptr;
	char_struct_ptr = (typeof(char_struct_ptr))char_ptr;
	/* expect+1: warning: pointer cast from 'pointer to signed char' to 'pointer to struct typedef char_struct' may be troublesome [247] */
	char_struct_ptr = (typeof(char_struct_ptr))schar_ptr;
	char_struct_ptr = (typeof(char_struct_ptr))uchar_ptr;
	/* expect+1: warning: pointer cast from 'pointer to short' to 'pointer to struct typedef char_struct' may be troublesome [247] */
	char_struct_ptr = (typeof(char_struct_ptr))short_ptr;
	/* expect+1: warning: pointer cast from 'pointer to unsigned short' to 'pointer to struct typedef char_struct' may be troublesome [247] */
	char_struct_ptr = (typeof(char_struct_ptr))ushort_ptr;
	/* expect+1: warning: pointer cast from 'pointer to int' to 'pointer to struct typedef char_struct' may be troublesome [247] */
	char_struct_ptr = (typeof(char_struct_ptr))int_ptr;
	/* expect+1: warning: pointer cast from 'pointer to unsigned int' to 'pointer to struct typedef char_struct' may be troublesome [247] */
	char_struct_ptr = (typeof(char_struct_ptr))uint_ptr;
	/* expect+1: warning: pointer cast from 'pointer to long' to 'pointer to struct typedef char_struct' may be troublesome [247] */
	char_struct_ptr = (typeof(char_struct_ptr))long_ptr;
	/* expect+1: warning: pointer cast from 'pointer to unsigned long' to 'pointer to struct typedef char_struct' may be troublesome [247] */
	char_struct_ptr = (typeof(char_struct_ptr))ulong_ptr;
	/* expect+1: warning: pointer cast from 'pointer to long long' to 'pointer to struct typedef char_struct' may be troublesome [247] */
	char_struct_ptr = (typeof(char_struct_ptr))llong_ptr;
	/* expect+1: warning: pointer cast from 'pointer to unsigned long long' to 'pointer to struct typedef char_struct' may be troublesome [247] */
	char_struct_ptr = (typeof(char_struct_ptr))ullong_ptr;
	/* expect+1: warning: pointer cast from 'pointer to float' to 'pointer to struct typedef char_struct' may be troublesome [247] */
	char_struct_ptr = (typeof(char_struct_ptr))float_ptr;
	/* expect+1: warning: pointer cast from 'pointer to double' to 'pointer to struct typedef char_struct' may be troublesome [247] */
	char_struct_ptr = (typeof(char_struct_ptr))double_ptr;
	/* expect+1: warning: pointer cast from 'pointer to long double' to 'pointer to struct typedef char_struct' may be troublesome [247] */
	char_struct_ptr = (typeof(char_struct_ptr))ldouble_ptr;
	/* expect+1: warning: pointer cast from 'pointer to float _Complex' to 'pointer to struct typedef char_struct' may be troublesome [247] */
	char_struct_ptr = (typeof(char_struct_ptr))fcomplex_ptr;
	/* expect+1: warning: pointer cast from 'pointer to double _Complex' to 'pointer to struct typedef char_struct' may be troublesome [247] */
	char_struct_ptr = (typeof(char_struct_ptr))dcomplex_ptr;
	/* expect+1: warning: pointer cast from 'pointer to long double _Complex' to 'pointer to struct typedef char_struct' may be troublesome [247] */
	char_struct_ptr = (typeof(char_struct_ptr))lcomplex_ptr;
	char_struct_ptr = (typeof(char_struct_ptr))void_ptr;
	char_struct_ptr = (typeof(char_struct_ptr))char_struct_ptr;
	/* expect+1: warning: pointer cast from 'pointer to struct typedef double_struct' to 'pointer to struct typedef char_struct' may be troublesome [247] */
	char_struct_ptr = (typeof(char_struct_ptr))double_struct_ptr;
	/* expect+1: warning: pointer cast from 'pointer to union typedef char_union' to 'pointer to struct typedef char_struct' may be troublesome [247] */
	char_struct_ptr = (typeof(char_struct_ptr))char_union_ptr;
	/* expect+1: warning: pointer cast from 'pointer to union typedef double_union' to 'pointer to struct typedef char_struct' may be troublesome [247] */
	char_struct_ptr = (typeof(char_struct_ptr))double_union_ptr;
	/* expect+1: warning: pointer cast from 'pointer to enum typedef int_enum' to 'pointer to struct typedef char_struct' may be troublesome [247] */
	char_struct_ptr = (typeof(char_struct_ptr))enum_ptr;
	/* expect+1: warning: pointer cast from 'pointer to array[5] of double' to 'pointer to struct typedef char_struct' may be troublesome [247] */
	char_struct_ptr = (typeof(char_struct_ptr))double_array_ptr;
	/* expect+1: warning: converting 'pointer to function(void) returning void' to 'pointer to struct typedef char_struct' is questionable [229] */
	char_struct_ptr = (typeof(char_struct_ptr))func_ptr;

	/* expect+1: warning: pointer cast from 'pointer to _Bool' to 'pointer to struct typedef double_struct' may be troublesome [247] */
	double_struct_ptr = (typeof(double_struct_ptr))bool_ptr;
	double_struct_ptr = (typeof(double_struct_ptr))char_ptr;
	/* expect+1: warning: pointer cast from 'pointer to signed char' to 'pointer to struct typedef double_struct' may be troublesome [247] */
	double_struct_ptr = (typeof(double_struct_ptr))schar_ptr;
	double_struct_ptr = (typeof(double_struct_ptr))uchar_ptr;
	/* expect+1: warning: pointer cast from 'pointer to short' to 'pointer to struct typedef double_struct' may be troublesome [247] */
	double_struct_ptr = (typeof(double_struct_ptr))short_ptr;
	/* expect+1: warning: pointer cast from 'pointer to unsigned short' to 'pointer to struct typedef double_struct' may be troublesome [247] */
	double_struct_ptr = (typeof(double_struct_ptr))ushort_ptr;
	/* expect+1: warning: pointer cast from 'pointer to int' to 'pointer to struct typedef double_struct' may be troublesome [247] */
	double_struct_ptr = (typeof(double_struct_ptr))int_ptr;
	/* expect+1: warning: pointer cast from 'pointer to unsigned int' to 'pointer to struct typedef double_struct' may be troublesome [247] */
	double_struct_ptr = (typeof(double_struct_ptr))uint_ptr;
	/* expect+1: warning: pointer cast from 'pointer to long' to 'pointer to struct typedef double_struct' may be troublesome [247] */
	double_struct_ptr = (typeof(double_struct_ptr))long_ptr;
	/* expect+1: warning: pointer cast from 'pointer to unsigned long' to 'pointer to struct typedef double_struct' may be troublesome [247] */
	double_struct_ptr = (typeof(double_struct_ptr))ulong_ptr;
	/* expect+1: warning: pointer cast from 'pointer to long long' to 'pointer to struct typedef double_struct' may be troublesome [247] */
	double_struct_ptr = (typeof(double_struct_ptr))llong_ptr;
	/* expect+1: warning: pointer cast from 'pointer to unsigned long long' to 'pointer to struct typedef double_struct' may be troublesome [247] */
	double_struct_ptr = (typeof(double_struct_ptr))ullong_ptr;
	/* expect+1: warning: pointer cast from 'pointer to float' to 'pointer to struct typedef double_struct' may be troublesome [247] */
	double_struct_ptr = (typeof(double_struct_ptr))float_ptr;
	/* expect+1: warning: pointer cast from 'pointer to double' to 'pointer to struct typedef double_struct' may be troublesome [247] */
	double_struct_ptr = (typeof(double_struct_ptr))double_ptr;
	/* expect+1: warning: pointer cast from 'pointer to long double' to 'pointer to struct typedef double_struct' may be troublesome [247] */
	double_struct_ptr = (typeof(double_struct_ptr))ldouble_ptr;
	/* expect+1: warning: pointer cast from 'pointer to float _Complex' to 'pointer to struct typedef double_struct' may be troublesome [247] */
	double_struct_ptr = (typeof(double_struct_ptr))fcomplex_ptr;
	/* expect+1: warning: pointer cast from 'pointer to double _Complex' to 'pointer to struct typedef double_struct' may be troublesome [247] */
	double_struct_ptr = (typeof(double_struct_ptr))dcomplex_ptr;
	/* expect+1: warning: pointer cast from 'pointer to long double _Complex' to 'pointer to struct typedef double_struct' may be troublesome [247] */
	double_struct_ptr = (typeof(double_struct_ptr))lcomplex_ptr;
	double_struct_ptr = (typeof(double_struct_ptr))void_ptr;
	/* expect+1: warning: pointer cast from 'pointer to struct typedef char_struct' to 'pointer to struct typedef double_struct' may be troublesome [247] */
	double_struct_ptr = (typeof(double_struct_ptr))char_struct_ptr;
	double_struct_ptr = (typeof(double_struct_ptr))double_struct_ptr;
	/* expect+1: warning: pointer cast from 'pointer to union typedef char_union' to 'pointer to struct typedef double_struct' may be troublesome [247] */
	double_struct_ptr = (typeof(double_struct_ptr))char_union_ptr;
	/* expect+1: warning: pointer cast from 'pointer to union typedef double_union' to 'pointer to struct typedef double_struct' may be troublesome [247] */
	double_struct_ptr = (typeof(double_struct_ptr))double_union_ptr;
	/* expect+1: warning: pointer cast from 'pointer to enum typedef int_enum' to 'pointer to struct typedef double_struct' may be troublesome [247] */
	double_struct_ptr = (typeof(double_struct_ptr))enum_ptr;
	/* expect+1: warning: pointer cast from 'pointer to array[5] of double' to 'pointer to struct typedef double_struct' may be troublesome [247] */
	double_struct_ptr = (typeof(double_struct_ptr))double_array_ptr;
	/* expect+1: warning: converting 'pointer to function(void) returning void' to 'pointer to struct typedef double_struct' is questionable [229] */
	double_struct_ptr = (typeof(double_struct_ptr))func_ptr;

	/* expect+1: warning: pointer cast from 'pointer to _Bool' to 'pointer to union typedef char_union' may be troublesome [247] */
	char_union_ptr = (typeof(char_union_ptr))bool_ptr;
	char_union_ptr = (typeof(char_union_ptr))char_ptr;
	/* expect+1: warning: pointer cast from 'pointer to signed char' to 'pointer to union typedef char_union' may be troublesome [247] */
	char_union_ptr = (typeof(char_union_ptr))schar_ptr;
	char_union_ptr = (typeof(char_union_ptr))uchar_ptr;
	/* expect+1: warning: pointer cast from 'pointer to short' to 'pointer to union typedef char_union' may be troublesome [247] */
	char_union_ptr = (typeof(char_union_ptr))short_ptr;
	/* expect+1: warning: pointer cast from 'pointer to unsigned short' to 'pointer to union typedef char_union' may be troublesome [247] */
	char_union_ptr = (typeof(char_union_ptr))ushort_ptr;
	/* expect+1: warning: pointer cast from 'pointer to int' to 'pointer to union typedef char_union' may be troublesome [247] */
	char_union_ptr = (typeof(char_union_ptr))int_ptr;
	/* expect+1: warning: pointer cast from 'pointer to unsigned int' to 'pointer to union typedef char_union' may be troublesome [247] */
	char_union_ptr = (typeof(char_union_ptr))uint_ptr;
	/* expect+1: warning: pointer cast from 'pointer to long' to 'pointer to union typedef char_union' may be troublesome [247] */
	char_union_ptr = (typeof(char_union_ptr))long_ptr;
	/* expect+1: warning: pointer cast from 'pointer to unsigned long' to 'pointer to union typedef char_union' may be troublesome [247] */
	char_union_ptr = (typeof(char_union_ptr))ulong_ptr;
	/* expect+1: warning: pointer cast from 'pointer to long long' to 'pointer to union typedef char_union' may be troublesome [247] */
	char_union_ptr = (typeof(char_union_ptr))llong_ptr;
	/* expect+1: warning: pointer cast from 'pointer to unsigned long long' to 'pointer to union typedef char_union' may be troublesome [247] */
	char_union_ptr = (typeof(char_union_ptr))ullong_ptr;
	/* expect+1: warning: pointer cast from 'pointer to float' to 'pointer to union typedef char_union' may be troublesome [247] */
	char_union_ptr = (typeof(char_union_ptr))float_ptr;
	/* expect+1: warning: pointer cast from 'pointer to double' to 'pointer to union typedef char_union' may be troublesome [247] */
	char_union_ptr = (typeof(char_union_ptr))double_ptr;
	/* expect+1: warning: pointer cast from 'pointer to long double' to 'pointer to union typedef char_union' may be troublesome [247] */
	char_union_ptr = (typeof(char_union_ptr))ldouble_ptr;
	/* expect+1: warning: pointer cast from 'pointer to float _Complex' to 'pointer to union typedef char_union' may be troublesome [247] */
	char_union_ptr = (typeof(char_union_ptr))fcomplex_ptr;
	/* expect+1: warning: pointer cast from 'pointer to double _Complex' to 'pointer to union typedef char_union' may be troublesome [247] */
	char_union_ptr = (typeof(char_union_ptr))dcomplex_ptr;
	/* expect+1: warning: pointer cast from 'pointer to long double _Complex' to 'pointer to union typedef char_union' may be troublesome [247] */
	char_union_ptr = (typeof(char_union_ptr))lcomplex_ptr;
	char_union_ptr = (typeof(char_union_ptr))void_ptr;
	/* expect+1: warning: pointer cast from 'pointer to struct typedef char_struct' to 'pointer to union typedef char_union' may be troublesome [247] */
	char_union_ptr = (typeof(char_union_ptr))char_struct_ptr;
	/* expect+1: warning: pointer cast from 'pointer to struct typedef double_struct' to 'pointer to union typedef char_union' may be troublesome [247] */
	char_union_ptr = (typeof(char_union_ptr))double_struct_ptr;
	char_union_ptr = (typeof(char_union_ptr))char_union_ptr;
	/* expect+1: warning: pointer cast from 'pointer to union typedef double_union' to 'pointer to union typedef char_union' may be troublesome [247] */
	char_union_ptr = (typeof(char_union_ptr))double_union_ptr;
	/* expect+1: warning: pointer cast from 'pointer to enum typedef int_enum' to 'pointer to union typedef char_union' may be troublesome [247] */
	char_union_ptr = (typeof(char_union_ptr))enum_ptr;
	/* expect+1: warning: pointer cast from 'pointer to array[5] of double' to 'pointer to union typedef char_union' may be troublesome [247] */
	char_union_ptr = (typeof(char_union_ptr))double_array_ptr;
	/* expect+1: warning: converting 'pointer to function(void) returning void' to 'pointer to union typedef char_union' is questionable [229] */
	char_union_ptr = (typeof(char_union_ptr))func_ptr;

	/* expect+1: warning: pointer cast from 'pointer to _Bool' to 'pointer to union typedef double_union' may be troublesome [247] */
	double_union_ptr = (typeof(double_union_ptr))bool_ptr;
	double_union_ptr = (typeof(double_union_ptr))char_ptr;
	/* expect+1: warning: pointer cast from 'pointer to signed char' to 'pointer to union typedef double_union' may be troublesome [247] */
	double_union_ptr = (typeof(double_union_ptr))schar_ptr;
	double_union_ptr = (typeof(double_union_ptr))uchar_ptr;
	/* expect+1: warning: pointer cast from 'pointer to short' to 'pointer to union typedef double_union' may be troublesome [247] */
	double_union_ptr = (typeof(double_union_ptr))short_ptr;
	/* expect+1: warning: pointer cast from 'pointer to unsigned short' to 'pointer to union typedef double_union' may be troublesome [247] */
	double_union_ptr = (typeof(double_union_ptr))ushort_ptr;
	/* expect+1: warning: pointer cast from 'pointer to int' to 'pointer to union typedef double_union' may be troublesome [247] */
	double_union_ptr = (typeof(double_union_ptr))int_ptr;
	/* expect+1: warning: pointer cast from 'pointer to unsigned int' to 'pointer to union typedef double_union' may be troublesome [247] */
	double_union_ptr = (typeof(double_union_ptr))uint_ptr;
	/* expect+1: warning: pointer cast from 'pointer to long' to 'pointer to union typedef double_union' may be troublesome [247] */
	double_union_ptr = (typeof(double_union_ptr))long_ptr;
	/* expect+1: warning: pointer cast from 'pointer to unsigned long' to 'pointer to union typedef double_union' may be troublesome [247] */
	double_union_ptr = (typeof(double_union_ptr))ulong_ptr;
	/* expect+1: warning: pointer cast from 'pointer to long long' to 'pointer to union typedef double_union' may be troublesome [247] */
	double_union_ptr = (typeof(double_union_ptr))llong_ptr;
	/* expect+1: warning: pointer cast from 'pointer to unsigned long long' to 'pointer to union typedef double_union' may be troublesome [247] */
	double_union_ptr = (typeof(double_union_ptr))ullong_ptr;
	/* expect+1: warning: pointer cast from 'pointer to float' to 'pointer to union typedef double_union' may be troublesome [247] */
	double_union_ptr = (typeof(double_union_ptr))float_ptr;
	double_union_ptr = (typeof(double_union_ptr))double_ptr;
	/* expect+1: warning: pointer cast from 'pointer to long double' to 'pointer to union typedef double_union' may be troublesome [247] */
	double_union_ptr = (typeof(double_union_ptr))ldouble_ptr;
	/* expect+1: warning: pointer cast from 'pointer to float _Complex' to 'pointer to union typedef double_union' may be troublesome [247] */
	double_union_ptr = (typeof(double_union_ptr))fcomplex_ptr;
	/* expect+1: warning: pointer cast from 'pointer to double _Complex' to 'pointer to union typedef double_union' may be troublesome [247] */
	double_union_ptr = (typeof(double_union_ptr))dcomplex_ptr;
	/* expect+1: warning: pointer cast from 'pointer to long double _Complex' to 'pointer to union typedef double_union' may be troublesome [247] */
	double_union_ptr = (typeof(double_union_ptr))lcomplex_ptr;
	double_union_ptr = (typeof(double_union_ptr))void_ptr;
	/* expect+1: warning: pointer cast from 'pointer to struct typedef char_struct' to 'pointer to union typedef double_union' may be troublesome [247] */
	double_union_ptr = (typeof(double_union_ptr))char_struct_ptr;
	/* expect+1: warning: pointer cast from 'pointer to struct typedef double_struct' to 'pointer to union typedef double_union' may be troublesome [247] */
	double_union_ptr = (typeof(double_union_ptr))double_struct_ptr;
	/* expect+1: warning: pointer cast from 'pointer to union typedef char_union' to 'pointer to union typedef double_union' may be troublesome [247] */
	double_union_ptr = (typeof(double_union_ptr))char_union_ptr;
	double_union_ptr = (typeof(double_union_ptr))double_union_ptr;
	/* expect+1: warning: pointer cast from 'pointer to enum typedef int_enum' to 'pointer to union typedef double_union' may be troublesome [247] */
	double_union_ptr = (typeof(double_union_ptr))enum_ptr;
	double_union_ptr = (typeof(double_union_ptr))double_array_ptr;
	/* expect+1: warning: converting 'pointer to function(void) returning void' to 'pointer to union typedef double_union' is questionable [229] */
	double_union_ptr = (typeof(double_union_ptr))func_ptr;

	/* expect+1: warning: pointer cast from 'pointer to _Bool' to 'pointer to enum typedef int_enum' may be troublesome [247] */
	enum_ptr = (typeof(enum_ptr))bool_ptr;
	enum_ptr = (typeof(enum_ptr))char_ptr;
	/* expect+1: warning: pointer cast from 'pointer to signed char' to 'pointer to enum typedef int_enum' may be troublesome [247] */
	enum_ptr = (typeof(enum_ptr))schar_ptr;
	enum_ptr = (typeof(enum_ptr))uchar_ptr;
	/* expect+1: warning: pointer cast from 'pointer to short' to 'pointer to enum typedef int_enum' may be troublesome [247] */
	enum_ptr = (typeof(enum_ptr))short_ptr;
	/* expect+1: warning: pointer cast from 'pointer to unsigned short' to 'pointer to enum typedef int_enum' may be troublesome [247] */
	enum_ptr = (typeof(enum_ptr))ushort_ptr;
	enum_ptr = (typeof(enum_ptr))int_ptr;
	enum_ptr = (typeof(enum_ptr))uint_ptr;
	/* expect+1: warning: pointer cast from 'pointer to long' to 'pointer to enum typedef int_enum' may be troublesome [247] */
	enum_ptr = (typeof(enum_ptr))long_ptr;
	/* expect+1: warning: pointer cast from 'pointer to unsigned long' to 'pointer to enum typedef int_enum' may be troublesome [247] */
	enum_ptr = (typeof(enum_ptr))ulong_ptr;
	/* expect+1: warning: pointer cast from 'pointer to long long' to 'pointer to enum typedef int_enum' may be troublesome [247] */
	enum_ptr = (typeof(enum_ptr))llong_ptr;
	/* expect+1: warning: pointer cast from 'pointer to unsigned long long' to 'pointer to enum typedef int_enum' may be troublesome [247] */
	enum_ptr = (typeof(enum_ptr))ullong_ptr;
	/* expect+1: warning: pointer cast from 'pointer to float' to 'pointer to enum typedef int_enum' may be troublesome [247] */
	enum_ptr = (typeof(enum_ptr))float_ptr;
	/* expect+1: warning: pointer cast from 'pointer to double' to 'pointer to enum typedef int_enum' may be troublesome [247] */
	enum_ptr = (typeof(enum_ptr))double_ptr;
	/* expect+1: warning: pointer cast from 'pointer to long double' to 'pointer to enum typedef int_enum' may be troublesome [247] */
	enum_ptr = (typeof(enum_ptr))ldouble_ptr;
	/* expect+1: warning: pointer cast from 'pointer to float _Complex' to 'pointer to enum typedef int_enum' may be troublesome [247] */
	enum_ptr = (typeof(enum_ptr))fcomplex_ptr;
	/* expect+1: warning: pointer cast from 'pointer to double _Complex' to 'pointer to enum typedef int_enum' may be troublesome [247] */
	enum_ptr = (typeof(enum_ptr))dcomplex_ptr;
	/* expect+1: warning: pointer cast from 'pointer to long double _Complex' to 'pointer to enum typedef int_enum' may be troublesome [247] */
	enum_ptr = (typeof(enum_ptr))lcomplex_ptr;
	enum_ptr = (typeof(enum_ptr))void_ptr;
	/* expect+1: warning: pointer cast from 'pointer to struct typedef char_struct' to 'pointer to enum typedef int_enum' may be troublesome [247] */
	enum_ptr = (typeof(enum_ptr))char_struct_ptr;
	/* expect+1: warning: pointer cast from 'pointer to struct typedef double_struct' to 'pointer to enum typedef int_enum' may be troublesome [247] */
	enum_ptr = (typeof(enum_ptr))double_struct_ptr;
	/* expect+1: warning: pointer cast from 'pointer to union typedef char_union' to 'pointer to enum typedef int_enum' may be troublesome [247] */
	enum_ptr = (typeof(enum_ptr))char_union_ptr;
	/* expect+1: warning: pointer cast from 'pointer to union typedef double_union' to 'pointer to enum typedef int_enum' may be troublesome [247] */
	enum_ptr = (typeof(enum_ptr))double_union_ptr;
	enum_ptr = (typeof(enum_ptr))enum_ptr;
	/* expect+1: warning: pointer cast from 'pointer to array[5] of double' to 'pointer to enum typedef int_enum' may be troublesome [247] */
	enum_ptr = (typeof(enum_ptr))double_array_ptr;
	/* expect+1: warning: converting 'pointer to function(void) returning void' to 'pointer to enum typedef int_enum' is questionable [229] */
	enum_ptr = (typeof(enum_ptr))func_ptr;

	/* expect+1: warning: pointer cast from 'pointer to _Bool' to 'pointer to array[5] of double' may be troublesome [247] */
	double_array_ptr = (typeof(double_array_ptr))bool_ptr;
	double_array_ptr = (typeof(double_array_ptr))char_ptr;
	/* expect+1: warning: pointer cast from 'pointer to signed char' to 'pointer to array[5] of double' may be troublesome [247] */
	double_array_ptr = (typeof(double_array_ptr))schar_ptr;
	double_array_ptr = (typeof(double_array_ptr))uchar_ptr;
	/* expect+1: warning: pointer cast from 'pointer to short' to 'pointer to array[5] of double' may be troublesome [247] */
	double_array_ptr = (typeof(double_array_ptr))short_ptr;
	/* expect+1: warning: pointer cast from 'pointer to unsigned short' to 'pointer to array[5] of double' may be troublesome [247] */
	double_array_ptr = (typeof(double_array_ptr))ushort_ptr;
	/* expect+1: warning: pointer cast from 'pointer to int' to 'pointer to array[5] of double' may be troublesome [247] */
	double_array_ptr = (typeof(double_array_ptr))int_ptr;
	/* expect+1: warning: pointer cast from 'pointer to unsigned int' to 'pointer to array[5] of double' may be troublesome [247] */
	double_array_ptr = (typeof(double_array_ptr))uint_ptr;
	/* expect+1: warning: pointer cast from 'pointer to long' to 'pointer to array[5] of double' may be troublesome [247] */
	double_array_ptr = (typeof(double_array_ptr))long_ptr;
	/* expect+1: warning: pointer cast from 'pointer to unsigned long' to 'pointer to array[5] of double' may be troublesome [247] */
	double_array_ptr = (typeof(double_array_ptr))ulong_ptr;
	/* expect+1: warning: pointer cast from 'pointer to long long' to 'pointer to array[5] of double' may be troublesome [247] */
	double_array_ptr = (typeof(double_array_ptr))llong_ptr;
	/* expect+1: warning: pointer cast from 'pointer to unsigned long long' to 'pointer to array[5] of double' may be troublesome [247] */
	double_array_ptr = (typeof(double_array_ptr))ullong_ptr;
	/* expect+1: warning: pointer cast from 'pointer to float' to 'pointer to array[5] of double' may be troublesome [247] */
	double_array_ptr = (typeof(double_array_ptr))float_ptr;
	double_array_ptr = (typeof(double_array_ptr))double_ptr;
	/* expect+1: warning: pointer cast from 'pointer to long double' to 'pointer to array[5] of double' may be troublesome [247] */
	double_array_ptr = (typeof(double_array_ptr))ldouble_ptr;
	/* expect+1: warning: pointer cast from 'pointer to float _Complex' to 'pointer to array[5] of double' may be troublesome [247] */
	double_array_ptr = (typeof(double_array_ptr))fcomplex_ptr;
	/* expect+1: warning: pointer cast from 'pointer to double _Complex' to 'pointer to array[5] of double' may be troublesome [247] */
	double_array_ptr = (typeof(double_array_ptr))dcomplex_ptr;
	/* expect+1: warning: pointer cast from 'pointer to long double _Complex' to 'pointer to array[5] of double' may be troublesome [247] */
	double_array_ptr = (typeof(double_array_ptr))lcomplex_ptr;
	double_array_ptr = (typeof(double_array_ptr))void_ptr;
	/* expect+1: warning: pointer cast from 'pointer to struct typedef char_struct' to 'pointer to array[5] of double' may be troublesome [247] */
	double_array_ptr = (typeof(double_array_ptr))char_struct_ptr;
	/* expect+1: warning: pointer cast from 'pointer to struct typedef double_struct' to 'pointer to array[5] of double' may be troublesome [247] */
	double_array_ptr = (typeof(double_array_ptr))double_struct_ptr;
	/* expect+1: warning: pointer cast from 'pointer to union typedef char_union' to 'pointer to array[5] of double' may be troublesome [247] */
	double_array_ptr = (typeof(double_array_ptr))char_union_ptr;
	double_array_ptr = (typeof(double_array_ptr))double_union_ptr;
	/* expect+1: warning: pointer cast from 'pointer to enum typedef int_enum' to 'pointer to array[5] of double' may be troublesome [247] */
	double_array_ptr = (typeof(double_array_ptr))enum_ptr;
	double_array_ptr = (typeof(double_array_ptr))double_array_ptr;
	/* expect+1: warning: converting 'pointer to function(void) returning void' to 'pointer to array[5] of double' is questionable [229] */
	double_array_ptr = (typeof(double_array_ptr))func_ptr;

	/* expect+1: warning: converting 'pointer to _Bool' to 'pointer to function(void) returning void' is questionable [229] */
	func_ptr = (typeof(func_ptr))bool_ptr;
	/* expect+1: warning: converting 'pointer to char' to 'pointer to function(void) returning void' is questionable [229] */
	func_ptr = (typeof(func_ptr))char_ptr;
	/* expect+1: warning: converting 'pointer to signed char' to 'pointer to function(void) returning void' is questionable [229] */
	func_ptr = (typeof(func_ptr))schar_ptr;
	/* expect+1: warning: converting 'pointer to unsigned char' to 'pointer to function(void) returning void' is questionable [229] */
	func_ptr = (typeof(func_ptr))uchar_ptr;
	/* expect+1: warning: converting 'pointer to short' to 'pointer to function(void) returning void' is questionable [229] */
	func_ptr = (typeof(func_ptr))short_ptr;
	/* expect+1: warning: converting 'pointer to unsigned short' to 'pointer to function(void) returning void' is questionable [229] */
	func_ptr = (typeof(func_ptr))ushort_ptr;
	/* expect+1: warning: converting 'pointer to int' to 'pointer to function(void) returning void' is questionable [229] */
	func_ptr = (typeof(func_ptr))int_ptr;
	/* expect+1: warning: converting 'pointer to unsigned int' to 'pointer to function(void) returning void' is questionable [229] */
	func_ptr = (typeof(func_ptr))uint_ptr;
	/* expect+1: warning: converting 'pointer to long' to 'pointer to function(void) returning void' is questionable [229] */
	func_ptr = (typeof(func_ptr))long_ptr;
	/* expect+1: warning: converting 'pointer to unsigned long' to 'pointer to function(void) returning void' is questionable [229] */
	func_ptr = (typeof(func_ptr))ulong_ptr;
	/* expect+1: warning: converting 'pointer to long long' to 'pointer to function(void) returning void' is questionable [229] */
	func_ptr = (typeof(func_ptr))llong_ptr;
	/* expect+1: warning: converting 'pointer to unsigned long long' to 'pointer to function(void) returning void' is questionable [229] */
	func_ptr = (typeof(func_ptr))ullong_ptr;
	/* expect+1: warning: converting 'pointer to float' to 'pointer to function(void) returning void' is questionable [229] */
	func_ptr = (typeof(func_ptr))float_ptr;
	/* expect+1: warning: converting 'pointer to double' to 'pointer to function(void) returning void' is questionable [229] */
	func_ptr = (typeof(func_ptr))double_ptr;
	/* expect+1: warning: converting 'pointer to long double' to 'pointer to function(void) returning void' is questionable [229] */
	func_ptr = (typeof(func_ptr))ldouble_ptr;
	/* expect+1: warning: converting 'pointer to float _Complex' to 'pointer to function(void) returning void' is questionable [229] */
	func_ptr = (typeof(func_ptr))fcomplex_ptr;
	/* expect+1: warning: converting 'pointer to double _Complex' to 'pointer to function(void) returning void' is questionable [229] */
	func_ptr = (typeof(func_ptr))dcomplex_ptr;
	/* expect+1: warning: converting 'pointer to long double _Complex' to 'pointer to function(void) returning void' is questionable [229] */
	func_ptr = (typeof(func_ptr))lcomplex_ptr;
	func_ptr = (typeof(func_ptr))void_ptr;
	/* expect+1: warning: converting 'pointer to struct typedef char_struct' to 'pointer to function(void) returning void' is questionable [229] */
	func_ptr = (typeof(func_ptr))char_struct_ptr;
	/* expect+1: warning: converting 'pointer to struct typedef double_struct' to 'pointer to function(void) returning void' is questionable [229] */
	func_ptr = (typeof(func_ptr))double_struct_ptr;
	/* expect+1: warning: converting 'pointer to union typedef char_union' to 'pointer to function(void) returning void' is questionable [229] */
	func_ptr = (typeof(func_ptr))char_union_ptr;
	/* expect+1: warning: converting 'pointer to union typedef double_union' to 'pointer to function(void) returning void' is questionable [229] */
	func_ptr = (typeof(func_ptr))double_union_ptr;
	/* expect+1: warning: converting 'pointer to enum typedef int_enum' to 'pointer to function(void) returning void' is questionable [229] */
	func_ptr = (typeof(func_ptr))enum_ptr;
	/* expect+1: warning: converting 'pointer to array[5] of double' to 'pointer to function(void) returning void' is questionable [229] */
	func_ptr = (typeof(func_ptr))double_array_ptr;
	func_ptr = (typeof(func_ptr))func_ptr;
}
