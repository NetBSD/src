# 2 "d_fold_test.c"

/*
 * Test how expressions are handled in a context where they are tested for
 * truthiness, such as in the condition of an if statement.
 */

/* lint1-extra-flags: -X 351 */

struct s {
	int member;
};

union u {
	int member;
};

enum e {
	E
};

struct arr {
	int arr[4];
};

/* C99 6.2.5p2 */
void if_Bool(_Bool b)			{ if (b) return; }

/* C99 6.2.5p3 */
void if_char(char c)			{ if (c) return; }

/* C99 6.2.5p4 */
void if_signed_char(signed char sc)	{ if (sc) return; }
void if_short_int(short s)		{ if (s) return; }
void if_int(int i)			{ if (i) return; }
void if_long_int(long int l)		{ if (l) return; }
void if_long_long_int(long long int ll)	{ if (ll) return; }

/* C99 6.2.5p6 */
void if_unsigned_char(unsigned char uc)			{ if (uc) return; }
void if_unsigned_short_int(unsigned short us)		{ if (us) return; }
void if_unsigned_int(unsigned int ui)			{ if (ui) return; }
void if_unsigned_long_int(unsigned long int ul)		{ if (ul) return; }
void if_unsigned_long_long_int(unsigned long long int ull) { if (ull) return; }

/* C99 6.2.5p10 */
void if_float(float f)			{ if (f) return; }
void if_double(double d)		{ if (d) return; }
void if_long_double(long double ld)	{ if (ld) return; }

/* C99 6.2.5p11 */
void if_float_Complex(float _Complex fc)		{ if (fc) return; }
void if_double_Complex(double _Complex dc)		{ if (dc) return; }
void if_long_double_Complex(long double _Complex ldc)	{ if (ldc) return; }

/* C99 6.2.5p16 */
void if_enum(enum e e)			{ if (e) return; }

/* C99 6.2.5p20 */
void if_array(struct arr arr)		{ if (arr.arr) return; }
/* expect+2: error: controlling expressions must have scalar type [204] */
/* expect+1: warning: parameter 's' unused in function 'if_struct' [231] */
void if_struct(struct s s)		{ if (s) return; }
/* expect+2: error: controlling expressions must have scalar type [204] */
/* expect+1: warning: parameter 'u' unused in function 'if_union' [231] */
void if_union(union u u)		{ if (u) return; }
void if_function(void)			{ if (if_function) return; }
void if_pointer(void *p)		{ if (p) return; }

/* C99 6.8.5 */
/* expect+2: error: controlling expressions must have scalar type [204] */
/* expect+1: warning: parameter 's' unused in function 'while_struct' [231] */
void while_struct(struct s s)		{ while (s) return; }
/* expect+3: error: controlling expressions must have scalar type [204] */
/* expect+2: warning: end-of-loop code not reached [223] */
/* expect+1: warning: parameter 's' unused in function 'for_struct' [231] */
void for_struct(struct s s)		{ for (;s;) return; }
/* expect+2: error: controlling expressions must have scalar type [204] */
/* expect+1: warning: parameter 's' unused in function 'do_while_struct' [231] */
void do_while_struct(struct s s)	{ do { return; } while (s); }

/* C99 6.5.15 does not require a scalar type, curiously. */
/* expect+3: error: first operand of '?' must have scalar type [170] */
/* expect+2: warning: function 'conditional_struct' expects to return value [214] */
/* expect+1: warning: parameter 's' unused in function 'conditional_struct' [231] */
int conditional_struct(struct s s)	{ return s ? 1 : 2; }
