/*	$NetBSD: msg_267.c,v 1.8 2024/03/12 07:56:08 rillig Exp $	*/
# 3 "msg_267.c"

// Test for message: shift amount %u equals bit-size of '%s' [267]

/* lint1-extra-flags: -X 351 */

int
shr32(unsigned int x)
{
	/* expect+1: warning: shift amount 32 equals bit-size of 'unsigned int' [267] */
	return x >> 32;
}

int
shl32(unsigned int x)
{
	/* expect+1: warning: shift amount 32 equals bit-size of 'unsigned int' [267] */
	return x << 32;
}

/*
 * As of 2022-08-19, lint ignores the GCC-specific 'mode' attribute, treating
 * the tetra-int as a plain single-int, thus having width 32.
 *
 * https://gcc.gnu.org/onlinedocs/gccint/Machine-Modes.html
 */
unsigned
function(unsigned __attribute__((mode(TI))) arg)
{
	/* expect+1: warning: shift amount 32 equals bit-size of 'unsigned int' [267] */
	return (arg >> 32) & 3;
}

unsigned
shift_bit_field(void)
{
	struct {
		unsigned bit_field:18;
	} s = { 12345 };

	/*
	 * A warning may be useful here for '>>' with a shift amount >= 18.
	 *
	 * For '<<' and bit-size <= 31, a warning only makes sense for shift
	 * amounts >= 31, as it is legitimate to rely on the default integer
	 * promotions of the left-hand operand. The default integer promotion
	 * turns the type into 'int', not 'unsigned int', therefore the 31.
	 * Using the same warning text would be confusing though.
	 *
	 * For '<<' and bit-size == 32, the standard case applies.
	 *
	 * As of 2022-08-19, Clang-tidy doesn't warn about any of these.
	 */
	return
	    (s.bit_field >> 17) &
	    (s.bit_field >> 18) &
	    (s.bit_field >> 19) &
	    (s.bit_field >> 31) &
	    // When promoting 'unsigned int:18', the target type is 'int', as
	    // it can represent all possible values; this is a bit misleading
	    // as its sign bit is always 0.
	    /* expect+1: warning: shift amount 32 equals bit-size of 'int:19' [267] */
	    (s.bit_field >> 32) &
	    // When promoting 'unsigned int:18', the target type is 'int', as
	    // it can represent all possible values; this is a bit misleading
	    // as its sign bit is always 0.
	    /* expect+1: warning: shift amount 33 is greater than bit-size 32 of 'int' [122] */
	    (s.bit_field >> 33) &
	    (s.bit_field << 17) &
	    (s.bit_field << 18) &
	    (s.bit_field << 19) &
	    (s.bit_field << 31) &
	    // When promoting 'unsigned int:18', the target type is 'int', as
	    // it can represent all possible values; this is a bit misleading
	    // as its sign bit is always 0.
	    /* expect+1: warning: shift amount 32 equals bit-size of 'int:19' [267] */
	    (s.bit_field << 32) &
	    // When promoting 'unsigned int:18', the target type is 'int', as
	    // it can represent all possible values; this is a bit misleading
	    // as its sign bit is always 0.
	    /* expect+1: warning: shift amount 33 is greater than bit-size 32 of 'int' [122] */
	    (s.bit_field << 33) &
	    15;
}
