/*	$NetBSD: d_alignof.c,v 1.12 2023/07/07 19:45:22 rillig Exp $	*/
# 3 "d_alignof.c"

/* https://gcc.gnu.org/onlinedocs/gcc/Alignment.html */

/* lint1-extra-flags: -X 351 */

unsigned long
leading_and_trailing_alignof_type(void)
{
	return __alignof__(short);
}

unsigned long
leading_alignof_type(void)
{
	return __alignof(short);
}

unsigned long
plain_alignof_type(void)
{
	/* The plain word 'alignof' is not recognized by GCC. */
	/* expect+2: error: function 'alignof' implicitly declared to return int [215] */
	/* expect+1: error: syntax error 'short' [249] */
	return alignof(short);
}
/* expect-1: warning: function 'plain_alignof_type' falls off bottom without returning value [217] */

unsigned long
leading_and_trailing_alignof_expr(void)
{
	return __alignof__ 3;
}

unsigned long
leading_alignof_expr(void)
{
	return __alignof 3;
}

unsigned long
plain_alignof_expr(void)
{
	/* The plain word 'alignof' is not recognized by GCC. */
	/* expect+2: error: 'alignof' undefined [99] */
	/* expect+1: error: syntax error '3' [249] */
	return alignof 3;
}
/* expect-1: warning: function 'plain_alignof_expr' falls off bottom without returning value [217] */


/*
 * As with 'sizeof', the keyword '__alignof__' doesn't require parentheses
 * when followed by an expression.  This allows for the seemingly strange
 * '->' after the parentheses, which in fact is perfectly fine.
 *
 * The NetBSD style guide says "We parenthesize sizeof expressions", even
 * though it is misleading in edge cases like this.  The GCC manual says that
 * '__alignof__' and 'sizeof' are syntactically the same, therefore the same
 * reasoning applies to '__alignof__'.
 */
unsigned long
alignof_pointer_to_member(void)
{
	struct s {
		unsigned long member;
	} var = { 0 }, *ptr = &var;

	return __alignof__(ptr)->member + ptr->member;
}

void
alignof_variants(void)
{
	/* expect+1: error: negative array dimension (-4) [20] */
	typedef int array_int[-(int)__alignof(int[3])];

	/* expect+1: error: negative array dimension (-8) [20] */
	typedef int array_double[-(int)__alignof(double[3])];

	/* expect+1: error: cannot take size/alignment of function type 'function(int) returning int' [144] */
	typedef int func[-(int)__alignof(int(int))];

	struct int_double {
		int i;
		double d;
	};
	/* expect+1: error: negative array dimension (-8) [20] */
	typedef int struct_int_double[-(int)__alignof(struct int_double)];

	struct chars {
		char name[20];
	};
	/* expect+1: error: negative array dimension (-1) [20] */
	typedef int struct_chars[-(int)__alignof(struct chars)];

	/* expect+1: warning: struct 'incomplete_struct' never defined [233] */
	struct incomplete_struct;
	/* expect+1: error: cannot take size/alignment of incomplete type [143] */
	typedef int incomplete_struct[-(int)__alignof(struct incomplete_struct)];

	/* expect+1: warning: union 'incomplete_union' never defined [234] */
	union incomplete_union;
	/* expect+1: error: cannot take size/alignment of incomplete type [143] */
	typedef int incomplete_union[-(int)__alignof(union incomplete_union)];

	/* expect+1: warning: enum 'incomplete_enum' never defined [235] */
	enum incomplete_enum;
	/* expect+1: error: cannot take size/alignment of incomplete type [143] */
	typedef int incomplete_enum[-(int)__alignof(enum incomplete_enum)];

	/* expect+1: error: cannot take size/alignment of incomplete type [143] */
	typedef int incomplete_array[-(int)__alignof(int[])];

	struct bit_fields {
		_Bool bit_field:1;
	};
	/*
	 * FIXME: This is not an attempt to initialize the typedef, it's the
	 * initialization of a nested expression.
	 */
	/* expect+2: error: cannot initialize typedef '00000000_tmp' [25] */
	/* expect+1: error: cannot take size/alignment of bit-field [145] */
	typedef int bit_field_1[-(int)__alignof((struct bit_fields){0}.bit_field)];

	struct bit_fields bit_fields;
	/* expect+1: error: cannot take size/alignment of bit-field [145] */
	typedef int bit_field_2[-(int)__alignof(bit_fields.bit_field)];

	/* expect+1: error: cannot take size/alignment of void [146] */
	typedef int plain_void[-(int)__alignof(void)];
}
