/*	$NetBSD: queries.c,v 1.9 2023/01/15 14:00:09 rillig Exp $	*/
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
 * 	Finding code that intentionally suppresses a regular lint warning,
 * 	such as casts between arithmetic types.
 */

/* lint1-extra-flags: -q 1,2,3,4,5,6,7 */

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

void
Q3(int i, unsigned u)
{
	/* expect+1: implicit conversion changes sign from 'int' to 'unsigned int' [Q3] */
	u = i;

	/* expect+1: implicit conversion changes sign from 'unsigned int' to 'int' [Q3] */
	i = u;
}

unsigned long long
Q4(signed char *ptr, int i, unsigned long long ull)
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

extern void *allocate(void);

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
 * Since queries do not affect the exit status, force a warning to make this
 * test conform to the general expectation that a test that produces output
 * exits non-successfully.
 */
/* expect+1: warning: static variable 'unused' unused [226] */
static int unused;
