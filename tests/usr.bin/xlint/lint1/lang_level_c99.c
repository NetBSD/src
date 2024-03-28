/*	$NetBSD: lang_level_c99.c,v 1.3 2024/03/28 21:04:48 rillig Exp $	*/
# 3 "lang_level_c99.c"

/*
 * Tests that are specific to the C99 language level, in particular:
 *
 *	* syntax elements that were added in C99
 *	* lint diagnostics that differ between the C90 and C99 language levels
 *	* lint diagnostics that differ between the C99 and C11 language levels
 */

/* lint1-flags: -S -w -X 351 */

/*
 * Features that were added in the C99 standard, as listed in the C99 foreword.
 *
 * In the below comments, [-] means unsupported and [x] means supported.
 */

// [-] restricted character set support via digraphs and <iso646.h>
//
// Lint neither parses digraphs nor trigraphs.

// [x] wide character library support in <wchar.h> and <wctype.h>
//
// On all supported platforms, 'wchar_t' == 'int'.

const int wide_string[] = L"wide";

// [x] more precise aliasing rules via effective type
//
// Irrelevant, as lint does not check the runtime behavior.

// [x] restricted pointers
//
// Can be parsed, are otherwise ignored.

// [-] variable length arrays
//
// Variable length arrays are handled as if the number of elements in the array
// were always 1.

/* FIXME: Parameter 'n' _is_ actually used. */
/* expect+2: warning: parameter 'n' unused in function 'variable_length_arrays' [231] */
unsigned long long
variable_length_arrays(int n)
{
	int vla[n];
	/* FIXME: The array dimension is not constant, but still negative. */
	/* expect+1: error: negative array dimension (-4) [20] */
	typedef int sizeof_vla[-(int)sizeof(vla)];
	return sizeof(vla);
}

// [x] flexible array members
//
// Flexible array members are parsed but not validated thoroughly.

void
flexible_array_members(void)
{
	struct {
		int regular;
		int flexible[];
	} s = {
		0,
		// Flexible array member must not be initialized.  Lint does
		// not detect this, leaving the job to the C99 compiler.
		{ 1, 3, 4, }
	};
	/* expect+1: error: negative array dimension (-4) [20] */
	typedef int sizeof_s[-(int)sizeof(s)];
}

// [x] static and type qualifiers in parameter array declarators
//
// Can be parsed, are otherwise ignored.

// [-] complex (and imaginary) support in <complex.h>
//
// Lint does not keep track of which parts of a complex object are initialized.
//
// Lint does not support '_Imaginary'.

// [x] type-generic math macros in <tgmath.h>
//
// Irrelevant, as lint only sees the preprocessed source code.

// [x] the long long int type and library functions
//
// On all platforms supported by lint, 'long long' is 64 bits wide.  The other
// fixed-width types are 'char', 'short', 'int' and (only on 64-bit platforms)
// '__int128_t'.
//
// The lint standard libraries -lstdc and -lposix do not contain the
// functions added in C99.

/* expect+1: error: negative array dimension (-1) [20] */
typedef int sizeof_char[-(int)sizeof(char)];
/* expect+1: error: negative array dimension (-2) [20] */
typedef int sizeof_short[-(int)sizeof(short)];
/* expect+1: error: negative array dimension (-4) [20] */
typedef int sizeof_int[-(int)sizeof(int)];
/* expect+1: error: negative array dimension (-8) [20] */
typedef int sizeof_long_long[-(int)sizeof(long long)];

// [x] increased minimum translation limits
//
// Irrelevant, as lint does not have any hard-coded limits.

// [x] additional floating-point characteristics in <float.h>
//
// Lint has very limited support for floating point numbers, as it fully relies
// on the host platform.  This is noticeable when cross-compiling between
// platforms with different size or representation of 'long double'.

// [x] remove implicit int
//
// Lint parses old-style declarations and marks them as errors.

// [x] reliable integer division
//
// The lint source code requires a C99 compiler, so when mapping the integer
// operations to those from the host platform, lint uses these.

// [-] universal character names (\u and \U)
//
// No, as nothing in the NetBSD source tree uses this feature.

// [-] extended identifiers
//
// No, as nothing in the NetBSD source tree uses this feature.

// [x] hexadecimal floating-point constants and %a and %A printf/scanf
// conversion specifiers

void pf();			/* no prototype parameters */

void
hexadecimal_floating_point_constants(void)
{
	double hex = 0x1.0p34;
	pf("%s %a\n", "hex", hex);
}

// [x] compound literals
//
// See d_c99_compound_literal_comma.c.

// [x] designated initializers
//
// See d_c99_init.c.

// [x] // comments
//
// Also supported in GCC mode.

// [?] extended integer types and library functions in <inttypes.h> and
// <stdint.h>
//
// TODO

// [x] remove implicit function declaration

void
call_implicitly_declared_function(void)
{
	/* expect+1: error: function 'implicitly_declared_function' implicitly declared to return int [215] */
	implicitly_declared_function(0);
}

// [x] preprocessor arithmetic done in intmax_t/uintmax_t
//
// Irrelevant, as lint only sees the preprocessed source code.

// [x] mixed declarations and code

// [x] new block scopes for selection and iteration statements

// [?] integer constant type rules
//
// TODO

// [?] integer promotion rules
//
// TODO

// [x] macros with a variable number of arguments
//
// Irrelevant, as lint only sees the preprocessed source code.

// [x] the vscanf family of functions in <stdio.h> and <wchar.h>
//
// Irrelevant, as typical C99 compilers already check these.

// [x] additional math library functions in <math.h>
//
// Irrelevant, as lint does not check arithmetic expressions.
//
// Lint also does not generate its own standard library definition for libm.

// [x] treatment of error conditions by math library functions
// (math_errhandling)
//
// Irrelevant, as lint does not check for error handling.

// [x] floating-point environment access in <fenv.h>
//
// TODO

// [x] IEC 60559 (also known as IEC 559 or IEEE arithmetic) support
//
// On platforms that conform to IEC 60559, lint performs the arithmetic
// operations accordingly.  When cross-compiling on a vax host for other target
// platforms, no such support is available.

// [x] trailing comma allowed in enum declaration
//
// Yes, see the grammar rule 'enums_with_opt_comma'.

// [-] %lf conversion specifier allowed in printf
//
// TODO: see tests/lint2/msg_013.exp.

// [x] inline functions
//
// Yes, also allowed in GCC mode.

// [x] the snprintf family of functions in <stdio.h>
//
// The snprintf functions are treated like all other functions.  The checks for
// matching format strings targets traditional C only and thus does not apply
// to these functions, as they have a prototype definition.

// [x] boolean type in <stdbool.h>
//
// Yes.  Conversion to and from boolean follows 6.3.1.2.  See also the -T flag,
// which enables 'strict bool mode'.

// [x] idempotent type qualifiers
//
// Lint warns about duplicate type qualifiers but accepts them otherwise.

/* expect+1: warning: duplicate 'const' [10] */
const const int duplicate_type_qualifier = 2;

// [x] empty macro arguments
//
// Irrelevant, as lint only sees the preprocessed source code.

// [?] new structure type compatibility rules (tag compatibility)
//
// TODO

// [x] additional predefined macro names
//
// Irrelevant, as lint only sees the preprocessed source code.

// [-] _Pragma preprocessing operator
//
// No, not yet asked for.

// [-] standard pragmas
//
// No, not yet asked for.

// [x] __func__ predefined identifier
//
// Yes, see 'fallback_symbol'.

// [x] va_copy macro
//
// Irrelevant, as lint only sees the preprocessed source code.

// [x] additional strftime conversion specifiers
//
// Irrelevant, as lint does not check strftime in depth.

// [?] LIA compatibility annex
//
// TODO

// [x] deprecate ungetc at the beginning of a binary file
//
// Irrelevant, as lint's analysis is not that deep into the runtime behavior.

// [x] remove deprecation of aliased array parameters
//
// Irrelevant, as lint does not check for aliasing.

// [?] conversion of array to pointer not limited to lvalues
//
// TODO

// [x] relaxed constraints on aggregate and union initialization
//
// Yes, struct and union members can be initialized with non-constant
// expressions.  Members that have struct or union type can be initialized with
// an expression of the same type.

// [x] relaxed restrictions on portable header names
//
// Irrelevant, as lint only sees the preprocessed source code.

// [x] return without expression not permitted in function that returns a value
// (and vice versa)

void
return_no_expr(int x)
{
	x++;
	/* expect+1: error: void function 'return_no_expr' cannot return value [213] */
	return x;
}

int
return_expr(void)
{
	/* expect+1: error: function 'return_expr' expects to return value [214] */
	return;
}
