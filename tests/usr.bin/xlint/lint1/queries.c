/*	$NetBSD: queries.c,v 1.27 2024/03/30 19:12:37 rillig Exp $	*/
# 3 "queries.c"

/*
 * Demonstrate the case-by-case queries.  Unlike warnings, queries do not
 * point to questionable code but rather to code that may be interesting to
 * inspect manually on a case-by-case basis.
 *
 * Possible use cases are:
 *
 *	Understanding how C works internally, by making the usual arithmetic
 *	conversions visible.
 *
 *	Finding code that intentionally suppresses a regular lint warning,
 *	such as casts between arithmetic types.
 */

/* lint1-extra-flags: -q 1,2,3,4,5,6,7,8,9,10 */
/* lint1-extra-flags: -q 11,12,13,14,15,16,17,18,19 */
/* lint1-extra-flags: -X 351 */

typedef unsigned char u8_t;
typedef unsigned short u16_t;
typedef unsigned int u32_t;
typedef unsigned long long u64_t;
typedef signed char s8_t;
typedef signed short s16_t;
typedef signed int s32_t;
typedef signed long long s64_t;

typedef float f32_t;
typedef double f64_t;
typedef float _Complex c32_t;
typedef double _Complex c64_t;

typedef char *str_t;
typedef const char *cstr_t;
typedef volatile char *vstr_t;
typedef typeof(sizeof 0) size_t;

_Bool cond;

u8_t u8;
u16_t u16;
u32_t u32;
u64_t u64;

s8_t s8;
s16_t s16;
s32_t s32;
s64_t s64;

struct {
	unsigned u8:8;
	unsigned u9:9;
	unsigned u10:10;
	unsigned u32:32;
	int s8:8;
	int s9:9;
	int s10:10;
	int s32:32;
} bits;

f32_t f32;
f64_t f64;

c32_t c32;
c64_t c64;

char *str;
const char *cstr;
volatile char *vstr;

void *void_ptr;
const void *const_void_ptr;

int
Q1(double dbl)
{
	/* expect+1: implicit conversion from floating point 'double' to integer 'int' [Q1] */
	return dbl;
}

int
Q2(double dbl)
{
	/* expect+1: cast from floating point 'double' to integer 'int' [Q2] */
	return (int)dbl;
}

// The Q3 query triggers so often that it also occurs outside this function.
void
Q3(int i, unsigned u)
{
	/* expect+1: implicit conversion changes sign from 'int' to 'unsigned int' [Q3] */
	u = i;

	/* expect+1: implicit conversion changes sign from 'unsigned int' to 'int' [Q3] */
	i = u;

	/* expect+2: implicit conversion changes sign from 'unsigned char' to 'int' [Q3] */
	/* expect+1: implicit conversion changes sign from 'int' to 'unsigned short' [Q3] */
	u16 += u8;
	/* expect+2: implicit conversion changes sign from 'unsigned short' to 'int' [Q3] */
	/* expect+1: implicit conversion changes sign from 'int' to 'unsigned int' [Q3] */
	u32 += u16;
}

unsigned long long
Q4(signed char *ptr, int i, unsigned long long ull, size_t sz)
{

	/*
	 * For constants, the usual arithmetic conversions are usually not
	 * interesting, so omit them.
	 */
	u32 = u32 & 0xff;
	u32 &= 0xff;

	/* expect+2: usual arithmetic conversion for '&' from 'int' to 'unsigned int' [Q4] */
	/* expect+1: implicit conversion changes sign from 'int' to 'unsigned int' [Q3] */
	u32 = u32 & s32;
	/*
	 * XXX: C99 5.6.16.2 says that the usual arithmetic conversions
	 * happen for compound assignments as well.
	 */
	/* expect+1: implicit conversion changes sign from 'int' to 'unsigned int' [Q3] */
	u32 &= s32;

	/* expect+3: implicit conversion changes sign from 'unsigned char' to 'int' [Q3] */
	/* expect+2: usual arithmetic conversion for '&' from 'int' to 'unsigned int' [Q4] */
	/* expect+1: implicit conversion changes sign from 'int' to 'unsigned int' [Q3] */
	u32 = u32 & u8;

	s8 = ptr[sz];

	/*
	 * The conversion from 'signed char' to 'int' is done by the integer
	 * promotions (C11 6.3.1.1p2), not by the usual arithmetic
	 * conversions (C11 6.3.1.8p1).
	 */
	/* expect+2: usual arithmetic conversion for '+' from 'int' to 'unsigned long long' [Q4] */
	/* expect+1: implicit conversion changes sign from 'int' to 'unsigned long long' [Q3] */
	return ptr[0] + ptr[1] + i + ull;
}

void
Q5(signed char *ptr, int i)
{
	if (ptr + i > ptr)
		return;

	/* expect+1: pointer addition has integer on the left-hand side [Q5] */
	if (i + ptr > ptr)
		return;

	if (ptr[i] != '\0')
		return;

	/* expect+1: pointer addition has integer on the left-hand side [Q5] */
	if (i[ptr] != '\0')
		return;
}

void
Q6(int i)
{
	/* expect+1: no-op cast from 'int' to 'int' [Q6] */
	i = (int)4;

	/* expect+1: no-op cast from 'int' to 'int' [Q6] */
	i = (int)i + 1;
}

void *allocate(void);

void
Q7(void)
{

	/* expect+2: no-op cast from '_Bool' to '_Bool' [Q6] */
	/* expect+1: redundant cast from '_Bool' to '_Bool' before assignment [Q7] */
	cond = (_Bool)cond;
	cond = (_Bool)u8;
	u8 = (u8_t)cond;

	/* expect+2: no-op cast from 'unsigned char' to 'unsigned char' [Q6] */
	/* expect+1: redundant cast from 'unsigned char' to 'unsigned char' before assignment [Q7] */
	u8 = (u8_t)u8;
	u8 = (u8_t)u16;
	u8 = (u16_t)u8;
	/* expect+1: no-op cast from 'unsigned short' to 'unsigned short' [Q6] */
	u8 = (u16_t)u16;
	/* expect+1: no-op cast from 'unsigned char' to 'unsigned char' [Q6] */
	u16 = (u8_t)u8;
	u16 = (u8_t)u16;
	/* expect+1: redundant cast from 'unsigned char' to 'unsigned short' before assignment [Q7] */
	u16 = (u16_t)u8;
	/* expect+2: no-op cast from 'unsigned short' to 'unsigned short' [Q6] */
	/* expect+1: redundant cast from 'unsigned short' to 'unsigned short' before assignment [Q7] */
	u16 = (u16_t)u16;

	/* Mixing signed and unsigned types. */
	u8 = (u8_t)s8;
	s8 = (s8_t)u8;
	/* expect+1: redundant cast from 'unsigned char' to 'short' before assignment [Q7] */
	s16 = (s16_t)u8;
	/* expect+1: redundant cast from 'signed char' to 'short' before assignment [Q7] */
	s16 = (s16_t)s8;


	/*
	 * Neither GCC nor Clang accept typeof(bit-field), as that would add
	 * unnecessary complexity.  Lint accepts it but silently discards the
	 * bit-field portion from the type; see dcs_add_type.
	 */
	/* expect+1: redundant cast from 'unsigned char' to 'unsigned int' before assignment [Q7] */
	bits.u9 = (typeof(bits.u9))u8;


	/* expect+2: no-op cast from 'float' to 'float' [Q6] */
	/* expect+1: redundant cast from 'float' to 'float' before assignment [Q7] */
	f32 = (f32_t)f32;
	f32 = (f32_t)f64;
	f32 = (f64_t)f32;
	/* expect+1: no-op cast from 'double' to 'double' [Q6] */
	f32 = (f64_t)f64;
	/* expect+1: no-op cast from 'float' to 'float' [Q6] */
	f64 = (f32_t)f32;
	f64 = (f32_t)f64;
	/* expect+1: redundant cast from 'float' to 'double' before assignment [Q7] */
	f64 = (f64_t)f32;
	/* expect+2: no-op cast from 'double' to 'double' [Q6] */
	/* expect+1: redundant cast from 'double' to 'double' before assignment [Q7] */
	f64 = (f64_t)f64;


	/* expect+2: no-op cast from 'float _Complex' to 'float _Complex' [Q6] */
	/* expect+1: redundant cast from 'float _Complex' to 'float _Complex' before assignment [Q7] */
	c32 = (c32_t)c32;
	c32 = (c32_t)c64;
	c32 = (c64_t)c32;
	/* expect+1: no-op cast from 'double _Complex' to 'double _Complex' [Q6] */
	c32 = (c64_t)c64;
	/* expect+1: no-op cast from 'float _Complex' to 'float _Complex' [Q6] */
	c64 = (c32_t)c32;
	c64 = (c32_t)c64;
	/* expect+1: redundant cast from 'float _Complex' to 'double _Complex' before assignment [Q7] */
	c64 = (c64_t)c32;
	/* expect+2: no-op cast from 'double _Complex' to 'double _Complex' [Q6] */
	/* expect+1: redundant cast from 'double _Complex' to 'double _Complex' before assignment [Q7] */
	c64 = (c64_t)c64;


	/* Mixing real and complex floating point types. */
	/* expect+1: no-op cast from 'float' to 'float' [Q6] */
	c32 = (f32_t)f32;
	c32 = (c32_t)f32;
	/* expect+1: no-op cast from 'float' to 'float' [Q6] */
	c64 = (f32_t)f32;
	c64 = (f64_t)f32;
	c64 = (c32_t)f32;
	c64 = (c64_t)f32;


	/*
	 * Converting a void pointer type to an object pointer type requires
	 * an explicit cast in C++, as it is a narrowing conversion. In C,
	 * that conversion is done implicitly.
	 */

	/* expect+1: redundant cast from 'pointer to void' to 'pointer to char' before assignment [Q7] */
	str = (char *)allocate();
	/* expect+1: redundant cast from 'pointer to void' to 'pointer to const char' before assignment [Q7] */
	cstr = (const char *)allocate();
	cstr = (char *)allocate();

	/* expect+2: no-op cast from 'pointer to char' to 'pointer to char' [Q6] */
	/* expect+1: redundant cast from 'pointer to char' to 'pointer to char' before assignment [Q7] */
	str = (str_t)str;
	str = (str_t)cstr;
	/* expect+1: warning: operands of '=' have incompatible pointer types to 'char' and 'const char' [128] */
	str = (cstr_t)str;
	/* expect+2: no-op cast from 'pointer to const char' to 'pointer to const char' [Q6] */
	/* expect+1: warning: operands of '=' have incompatible pointer types to 'char' and 'const char' [128] */
	str = (cstr_t)cstr;
	/* expect+1: no-op cast from 'pointer to char' to 'pointer to char' [Q6] */
	cstr = (str_t)str;
	cstr = (str_t)cstr;
	cstr = (cstr_t)str;
	/* expect+2: no-op cast from 'pointer to const char' to 'pointer to const char' [Q6] */
	/* expect+1: redundant cast from 'pointer to const char' to 'pointer to const char' before assignment [Q7] */
	cstr = (cstr_t)cstr;

	/* expect+2: no-op cast from 'pointer to char' to 'pointer to char' [Q6] */
	/* expect+1: redundant cast from 'pointer to char' to 'pointer to char' before assignment [Q7] */
	str = (str_t)str;
	str = (str_t)vstr;
	/* expect+1: warning: operands of '=' have incompatible pointer types to 'char' and 'volatile char' [128] */
	str = (vstr_t)str;
	/* expect+2: no-op cast from 'pointer to volatile char' to 'pointer to volatile char' [Q6] */
	/* expect+1: warning: operands of '=' have incompatible pointer types to 'char' and 'volatile char' [128] */
	str = (vstr_t)vstr;
	/* expect+1: no-op cast from 'pointer to char' to 'pointer to char' [Q6] */
	vstr = (str_t)str;
	vstr = (str_t)vstr;
	vstr = (vstr_t)str;
	/* expect+2: no-op cast from 'pointer to volatile char' to 'pointer to volatile char' [Q6] */
	/* expect+1: redundant cast from 'pointer to volatile char' to 'pointer to volatile char' before assignment [Q7] */
	vstr = (vstr_t)vstr;
}

/*
 * Octal numbers were common in the 1970s, especially on 36-bit machines.
 * 50 years later, they are still used in numeric file permissions.
 */
void
Q8(void)
{

	u16 = 0;
	u16 = 000000;
	/* expect+1: octal number '0644' [Q8] */
	u16 = 0644;
	/* expect+1: octal number '0000644' [Q8] */
	u16 = 0000644;
}

int
Q9(int x)
{
	switch (x) {
	case 0:
		return 0;
	case 1:
		/* expect+1: parenthesized return value [Q9] */
		return (0);
	case 2:
		return +(0);
	case 3:
		return -(13);
	case 4:
		/* expect+2: comma operator with types 'int' and 'int' [Q12] */
		/* expect+1: parenthesized return value [Q9] */
		return (0), (1);
	case 5:
		/* expect+2: comma operator with types 'int' and 'int' [Q12] */
		/* expect+1: parenthesized return value [Q9] */
		return (0, 1);
	case 6:
		/* expect+1: comma operator with types 'int' and 'int' [Q12] */
		return 0, 1;
	case 7:
		/* expect+1: implicit conversion from floating point 'double' to integer 'int' [Q1] */
		return 0.0;
	case 8:
		/* expect+2: parenthesized return value [Q9] */
		/* expect+1: implicit conversion from floating point 'double' to integer 'int' [Q1] */
		return (0.0);
	case 9:
		return
# 363 "queries.c" 3 4
		((void *)0)
# 365 "queries.c"
		/* expect+1: warning: illegal combination of integer 'int' and pointer 'pointer to void' [183] */
		;
	case 10:
		/* expect+1: warning: illegal combination of integer 'int' and pointer 'pointer to void' [183] */
		return (void *)(0);
	default:
		return 0;
	}
}

void
Q10(void)
{
	int a, b, c;

	/* expect+2: chained assignment with '=' and '=' [Q10] */
	/* expect+1: chained assignment with '=' and '=' [Q10] */
	a = b = c = 0;

	/* expect+2: chained assignment with '*=' and '-=' [Q10] */
	/* expect+1: chained assignment with '+=' and '*=' [Q10] */
	a += b *= c -= 0;
}

void
Q11(void)
{
	/* expect+1: static variable 'static_var_no_init' in function [Q11] */
	static int static_var_no_init;
	/* expect+1: static variable 'static_var_init' in function [Q11] */
	static int static_var_init = 1;

	static_var_no_init++;
	static_var_init++;
}

void
Q12(void)
{
	/* expect+1: comma operator with types 'void' and '_Bool' [Q12] */
	if (Q11(), cond)
		return;

	/* expect+5: implicit conversion changes sign from 'unsigned char' to 'int' [Q3] */
	/* expect+4: implicit conversion changes sign from 'int' to 'unsigned short' [Q3] */
	/* expect+3: implicit conversion changes sign from 'unsigned short' to 'int' [Q3] */
	/* expect+2: implicit conversion changes sign from 'int' to 'unsigned int' [Q3] */
	/* expect+1: comma operator with types 'unsigned short' and 'unsigned int' [Q12] */
	u16 += u8, u32 += u16;
}

/* expect+1: redundant 'extern' in function declaration of 'extern_Q13' [Q13] */
extern void extern_Q13(void);
void extern_Q13(void);
/* expect+1: redundant 'extern' in function declaration of 'extern_Q13' [Q13] */
extern void extern_Q13(void), *extern_ptr;

int
Q14(signed char sc, unsigned char uc, int wc)
{
	// Plain 'char' is platform-dependent, see queries-{schar,uchar}.c.

	if (sc == 'c' || sc == L'w' || sc == 92 || sc == 0)
		return 2;
	/* expect+4: implicit conversion changes sign from 'unsigned char' to 'int' [Q3] */
	/* expect+3: implicit conversion changes sign from 'unsigned char' to 'int' [Q3] */
	/* expect+2: implicit conversion changes sign from 'unsigned char' to 'int' [Q3] */
	/* expect+1: implicit conversion changes sign from 'unsigned char' to 'int' [Q3] */
	if (uc == 'c' || uc == L'w' || uc == 92 || uc == 0)
		return 3;
	if (wc == 'c' || wc == L'w' || wc == 92 || wc == 0)
		return 4;
	return 5;
}

void *
Q15(void)
{
	/* expect+1: implicit conversion from integer 0 to pointer 'pointer to void' [Q15] */
	void *ptr_from_int = 0;
	/* expect+1: implicit conversion from integer 0 to pointer 'pointer to void' [Q15] */
	void *ptr_from_uint = 0U;
	/* expect+1: implicit conversion from integer 0 to pointer 'pointer to void' [Q15] */
	void *ptr_from_long = 0L;

	ptr_from_int = &ptr_from_int;
	ptr_from_uint = &ptr_from_uint;
	ptr_from_long = &ptr_from_long;

	void_ptr = (void *)0;
	const_void_ptr = (const void *)0;

	/* expect+1: implicit conversion from integer 0 to pointer 'pointer to void' [Q15] */
	return 0;
}

/*
 * Even though C99 6.2.2p4 allows a 'static' declaration followed by a
 * non-'static' declaration, it may look confusing.
 */
static void Q16(void);
/* expect+3: 'Q16' was declared 'static', now non-'static' [Q16] */
/* expect+2: warning: static function 'Q16' unused [236] */
void
Q16(void)
{
}

/* expect+1: invisible character U+0009 in character constant [Q17] */
char Q17_char[] = { ' ', '\0', '	' };
/* expect+1: invisible character U+0009 in string literal [Q17] */
char Q17_char_string[] = " \0	";
/* expect+1: invisible character U+0009 in character constant [Q17] */
int Q17_wide[] = { L' ', L'\0', L'	' };
/* expect+1: invisible character U+0009 in string literal [Q17] */
int Q17_wide_string[] = L" \0	";

/* For Q18, see queries_schar.c and queries_uchar.c. */

void
convert_from_integer_to_floating(void)
{
	/* expect+1: implicit conversion from integer 'unsigned int' to floating point 'float' [Q19] */
	f32 = 0xffff0000;
	/* expect+1: implicit conversion from integer 'unsigned int' to floating point 'float' [Q19] */
	f32 = 0xffffffff;
	/* expect+1: implicit conversion from integer 'int' to floating point 'float' [Q19] */
	f32 = s32;
	/* expect+1: implicit conversion from integer 'unsigned int' to floating point 'float' [Q19] */
	f32 = u32;
	/* expect+1: implicit conversion from integer 'int' to floating point 'double' [Q19] */
	f64 = s32;
	/* expect+1: implicit conversion from integer 'unsigned int' to floating point 'double' [Q19] */
	f64 = u32;
	/* expect+1: implicit conversion from integer 'long long' to floating point 'double' [Q19] */
	f64 = s64;
	/* expect+1: implicit conversion from integer 'unsigned long long' to floating point 'double' [Q19] */
	f64 = u64;

	f32 = 0.0F;
	f32 = 0.0;
	f64 = 0.0;

	f64 = (double)0;
	f64 = (double)u32;
}

/*
 * Since queries do not affect the exit status, force a warning to make this
 * test conform to the general expectation that a test that produces output
 * exits non-successfully.
 */
/* expect+1: warning: static variable 'unused' unused [226] */
static int unused;
