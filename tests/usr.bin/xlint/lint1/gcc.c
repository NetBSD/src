/*	$NetBSD: gcc.c,v 1.1 2024/06/08 09:09:20 rillig Exp $	*/
# 3 "gcc.c"

/*
 * Miscellaneous tests that are specific to lint's GCC mode.
 */

/* lint1-extra-flags: -X 351 */

// Before C99 introduced __func__, GCC already had __FUNCTION__ with the same
// semantics.
const char *
gcc_function(void)
{
	// TODO: make __FUNCTION__ an array, then uncomment the code.
	//typedef int size[-(int)sizeof __FUNCTION__];

	return __FUNCTION__;
}

// Before C99 introduced designators in initializers, GCC already had them,
// although with a different syntax for struct/union members and with the
// a...b for ranges of array elements.
int array_range_initializers[256] = {
	[2] = 1,
	[3] = 2,
	[4 ... 5] = 3
};

_Bool dbl_isinf(double);

// Test that the GCC '__extension__' and '__typeof' are recognized.
void
extension_and_typeof(void)
{
	double __logbw = 1;
	if (__extension__(({
		__typeof((__logbw)) x_ = (__logbw);
		!dbl_isinf((x_));
	})))
		__logbw = 1;
}

int
range_in_case_label(int i)
{
	switch (i) {
	case 1 ... 40:		// This is a GCC extension.
		return 1;
	default:
		return 2;
	}
}
