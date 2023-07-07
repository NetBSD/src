/*	$NetBSD: gcc_cast_union.c,v 1.4 2023/07/07 19:45:22 rillig Exp $	*/
# 3 "gcc_cast_union.c"

/*
 * Test the GCC extension for casting to a union type.
 *
 * As of GCC 10.3.0, GCC only prints a generic warning without any details:
 *	error: cast to union type from type not present in union
 * No idea why it neither mentions the union type nor the actual type.
 *
 * https://gcc.gnu.org/onlinedocs/gcc/Cast-to-Union.html
 */

/* lint1-extra-flags: -e -X 351 */

union anything {
	_Bool m_bool;
	char m_char;
	signed char m_signed_char;
	unsigned char m_unsigned_char;
	short m_short;
	unsigned short m_unsigned_short;
	int m_int;
	unsigned int m_unsigned_int;
	long m_long;
	unsigned long m_unsigned_long;
	long long m_long_long;
	unsigned long long m_unsigned_long_long;
	/* skip __int128_t and __uint128_t for now */
	float m_float;
	double m_double;
	long double m_long_double;

	struct m_struct {
		int member;
	} m_struct;
	union m_union {
		int member;
	} m_union;
	enum m_enum1 {
		E1
	} m_enum1;
	enum m_enum2 {
		E2
	} m_enum2;
	const char *m_const_char_pointer;
	double m_double_array[5];
	void (*m_function)(void *);
	void (*m_function_varargs)(void *, ...);
	float _Complex m_float_complex;
	double _Complex m_double_complex;
	long double _Complex m_long_double_complex;
};

enum other_enum {
	OTHER
};

void
test(void)
{
	union anything any;

	any = (union anything)(_Bool)0;
	any = (union anything)(char)'\0';
	any = (union anything)(signed char)'\0';
	any = (union anything)(unsigned char)'\0';
	any = (union anything)(short)'\0';
	any = (union anything)(unsigned short)'\0';
	any = (union anything)(int)'\0';
	any = (union anything)(unsigned int)'\0';
	any = (union anything)(long)'\0';
	any = (union anything)(unsigned long)'\0';
	any = (union anything)(long long)'\0';
	any = (union anything)(unsigned long long)'\0';
	any = (union anything)0.0F;
	any = (union anything)0.0;
	any = (union anything)0.0L;
	any = (union anything)(struct m_struct){ 0 };
	any = (union anything)(union m_union){ 0 };
	any = (union anything)E1;
	any = (union anything)E2;
	/* GCC allows enum mismatch even with -Wenum-conversion */
	/* expect+1: error: type 'enum other_enum' is not a member of 'union anything' [329] */
	any = (union anything)OTHER;
	/* expect+1: error: type 'pointer to char' is not a member of 'union anything' [329] */
	any = (union anything)"char *";
	any = (union anything)(const char *)"char *";
	/* expect+1: error: type 'pointer to double' is not a member of 'union anything' [329] */
	any = (union anything)(double[5]){ 0.0, 1.0, 2.0, 3.0, 4.0 };
	/* expect+1: error: type 'pointer to double' is not a member of 'union anything' [329] */
	any = (union anything)(double[4]){ 0.0, 1.0, 2.0, 3.0 };
	/* expect+1: error: type 'pointer to int' is not a member of 'union anything' [329] */
	any = (union anything)(int[5]){ 0, 1, 2, 3, 4 };
	any = (union anything)(float _Complex)0.0F;
	any = (union anything)(double _Complex)0.0;
	any = (union anything)(long double _Complex)0.0L;

	any = any;
}
