/*	$NetBSD: msg_204.c,v 1.7 2022/06/16 21:24:41 rillig Exp $	*/
# 3 "msg_204.c"

// Test for message: controlling expressions must have scalar type [204]

/* Suppress message "argument '%s' unused in function '%s'". */
/* lint1-extra-flags: -X 231 */

extern void
extern_function(void);

void (*function_pointer)(void);

/*
 * Since func.c 1.39 from 2020-12-31 18:51:28, lint had produced an error
 * when a controlling expression was a function.  Pointers to functions were
 * ok though.
 */
void
bug_between_2020_12_31_and_2021_01_08(void)
{
	if (extern_function)
		extern_function();

	/*
	 * FIXME: For some reason, the ampersand is discarded in
	 *  build_address.  This only has a visible effect if the
	 *  t_spec in check_controlling_expression is evaluated too early,
	 *  as has been the case before func.c 1.52 from 2021-01-08.
	 */
	if (&extern_function)
		extern_function();

	/* This one has always been ok since pointers are scalar types. */
	if (function_pointer)
		function_pointer();
}

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
/* expect+1: error: controlling expressions must have scalar type [204] */
void if_struct(struct s s)		{ if (s) return; }
/* expect+1: error: controlling expressions must have scalar type [204] */
void if_union(union u u)		{ if (u) return; }
void if_function(void)			{ if (if_function) return; }
void if_pointer(void *p)		{ if (p) return; }

/* C99 6.8.5 */
/* expect+1: error: controlling expressions must have scalar type [204] */
void while_struct(struct s s)		{ while (s) return; }
/* expect+2: error: controlling expressions must have scalar type [204] */
/* expect+1: warning: end-of-loop code not reached [223] */
void for_struct(struct s s)		{ for (;s;) return; }
/* expect+1: error: controlling expressions must have scalar type [204] */
void do_while_struct(struct s s)	{ do { return; } while (s); }

/*
 * C99 6.5.15 for the '?:' operator does not explicitly mention that the
 * controlling expression must have a scalar type, curiously.
 */
/* expect+2: error: first operand must have scalar type, op ? : [170] */
/* expect+1: warning: function 'conditional_struct' expects to return value [214] */
int conditional_struct(struct s s)	{ return s ? 1 : 2; }
