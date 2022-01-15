/*	$NetBSD: d_c99_init.c,v 1.40 2022/01/15 14:22:03 rillig Exp $	*/
# 3 "d_c99_init.c"

/*
 * Test C99 initializers.
 *
 * See C99 6.7.8 "Initialization".
*/


void use(const void *);

typedef struct any {
	const void *value;
} any;


// C99 6.7.8p11 says "optionally enclosed in braces".  Whether this wording
// means "a single pair of braces" or "as many pairs of braces as you want"
// is left for interpretation to the reader.
int scalar_without_braces = 3;
int scalar_with_optional_braces = { 3 };
int scalar_with_too_many_braces = {{ 3 }};
/* expect+1: error: too many initializers [174] */
int scalar_with_too_many_initializers = { 3, 5 };


// See initialization_expr, 'handing over to INIT'.
void
struct_initialization_via_assignment(any arg)
{
	any local = arg;
	use(&local);
}


// See initialization_expr, initialization_init_array_from_string.
char static_duration[] = "static duration";
signed char static_duration_signed[] = "static duration";
unsigned char static_duration_unsigned[] = "static duration";
int static_duration_wchar[] = L"static duration";

// See init_expr.
void
initialization_by_braced_string(void)
{
	any local = { "hello" };
	use(&local);
}

void
initialization_by_redundantly_braced_string(void)
{
	any local = {{{{ "hello" }}}};
	use(&local);
}

/*
 * Only scalar expressions and string literals may be enclosed by additional
 * braces.  Since 'arg' is a struct, this is a compile-time error.
 */
void
initialization_with_too_many_braces(any arg)
{
	/* expect+1: error: cannot initialize 'pointer to const void' from 'struct any' [185] */
	any local = { arg };
	use(&arg);
}

// Some of the following examples are mentioned in the introduction comment
// in init.c.

int number = 12345;

int number_with_braces_and_comma = {
	12345,
};

int array_with_fixed_size[3] = {
	111,
	222,
	333,
	/* expect+1: error: too many array initializers, expected 3 [173] */
	444,
};

// See update_type_of_array_of_unknown_size.
int array_of_unknown_size[] = {
	111,
	222,
	333,
};

int array_flat[2][2] = {
	11,
	12,
	21,
	22
};

int array_nested[2][2] = {
	{
		11,
		12
	},
	{
		21,
		22
	}
};

int array_with_designators[] = {
	['1'] = 111,
	['5'] = 555,
	['9'] = 999
};

int array_with_some_designators[] = {
	['1'] = 111,
	222,
	['9'] = 999
};

struct point {
	int x;
	int y;
};

struct point point = {
	3,
	4
};

struct point point_with_designators = {
	.y = 4,
	.x = 3,
};

struct point point_with_mixed_designators = {
	.x = 3,
	4,
	/* expect+1: error: too many struct/union initializers [172] */
	5,
	.x = 3,
};

int array_with_designator[] = {
	111,
	/* expect+1: error: syntax error 'designator '.member' is only for struct/union' [249] */
	.member = 222,
	333,
};

/*
 * C99 6.7.8p11 says that the initializer of a scalar can be "optionally
 * enclosed in braces".  It does not explicitly set an upper limit on the
 * number of braces.  It also doesn't restrict the term "initializer" to only
 * mean the "outermost initializer".  6.7.8p13 defines that a brace for a
 * structure or union always means to descend into the type.  Both GCC 10 and
 * Clang 8 already warn about these extra braces, nevertheless there is
 * real-life code (the Postfix MTA) that exploits this corner case of the
 * standard.
 */
struct point scalar_with_several_braces = {
	{{{3}}},
	{{{{4}}}},
};

struct rectangle {
	struct point top_left;
	struct point bottom_right;
};

/* C99 6.7.8p18 */
struct rectangle screen = {
	.bottom_right = {
		1920,
		1080,
	}
};

/*
 * C99 6.7.8p22 says: At the _end_ of its initializer list, the array no
 * longer has incomplete type.
 */
struct point points[] = {
	{
		/*
		 * At this point, the size of the object 'points' is not known
		 * yet since its type is still incomplete.  Lint could warn
		 * about this, but GCC and Clang already do.
		 *
		 * Before init.c 1.179 from 2021.03.30, the type information
		 * of 'points' was set too early, resulting in a negative
		 * array size below.
		 */
		sizeof(int[-(int)sizeof(points)]),
		4
	}
};


struct triangle {
	struct point points[3];
};

struct pentagon {
	struct point points[5];
};

struct geometry {
	struct pentagon pentagons[6];
	struct triangle triangles[10];
	struct point points[3][5][2];
};

/*
 * Initialization of a complex struct containing nested arrays and nested
 * structs.
 */
struct geometry geometry = {
	.pentagons[0].points[4].x = 1,
	.points[0][0][0] = { 0, 0 },
	.points[2][4][1] = {301, 302 },
	/* expect+1: array subscript cannot be > 2: 3 */
	.points[3][0][0] = {3001, 3002 },
	/* expect+1: array subscript cannot be > 4: 5 */
	.points[0][5][0] = {501, 502 },
	/* expect+1: array subscript cannot be > 1: 2 */
	.points[0][0][2] = {21, 22 },
};

struct ends_with_unnamed_bit_field {
	int member;
	int:0;
} ends_with_unnamed_bit_field = {
	12345,
	/* expect+1: too many struct/union initializers */
	23456,
};

char prefixed_message[] = {
	'E', ':', ' ',
	/* expect+1: illegal combination of integer (char) and pointer */
	"message\n",
};

char message_with_suffix[] = {
	"message",
	/* The excess character is not detected by lint but by compilers. */
	'\n',
};

struct ten {
	int i0;
	int i1;
	int i2;
	int i3;
	int i4;
	int i5;
	int i6;
	int i7;
	int i8;
	int i9;
};

struct ten ten = {
	.i3 = 3,
	4,
	5,
	6,
};


/*
 * ISO C99 6.7.8 provides a large list of examples for initialization,
 * covering all tricky edge cases.
 */

int c99_6_7_8_p24_example1_i = 3.5;
double _Complex c99_6_7_8_p24_example1_c = 5 + 3 * 1.0fi;

int c99_6_7_8_p25_example2[] = { 1, 3, 5 };

int c99_6_7_8_p26_example3a[4][3] = {
	{ 1, 3, 5 },
	{ 2, 4, 6 },
	{ 3, 5, 7 },
};

int c99_6_7_8_p26_example3b[4][3] = {
	1, 3, 5, 2, 4, 6, 3, 5, 7
};

int c99_6_7_8_p27_example4[4][3] = {
	{ 1 }, { 2 }, { 3 }, { 4 }
};

struct {
	int a[3], b;
} c99_6_7_8_p28_example5[] = {
	{ 1 },
	2,
};

short c99_6_7_8_p29_example6a[4][3][2] = {
	{ 1 },
	{ 2, 3 },
	{ 4, 5, 6 },
};

short c99_6_7_8_p29_example6b[4][3][2] = {
	1, 0, 0, 0, 0, 0,
	2, 3, 0, 0, 0, 0,
	4, 5, 6, 0, 0, 0,
};

short c99_6_7_8_p29_example6c[4][3][2] = {
	{
		{ 1 },
	},
	{
		{ 2, 3 },
	},
	{
		{ 4, 5 },
		{ 6 },
	}
};

void
c99_6_7_8_p31_example7(void)
{
	typedef int A[];

	A a = { 1, 2 }, b = { 3, 4, 5 };

	/* expect+1: error: negative array dimension (-8) [20] */
	typedef int reveal_sizeof_a[-(int)(sizeof(a))];
	/* expect+1: error: negative array dimension (-12) [20] */
	typedef int reveal_sizeof_b[-(int)(sizeof(b))];
}

char c99_6_7_8_p32_example8_s1[] = "abc",
     c99_6_7_8_p32_example8_t1[3] = "abc";
char c99_6_7_8_p32_example8_s2[] = { 'a', 'b', 'c', '\0' },
     c99_6_7_8_p32_example8_t2[3] = { 'a', 'b', 'c' };
char *c99_6_7_8_p32_example8_p = "abc";

enum { member_one, member_two };
const char *c99_6_7_8_p33_example9[] = {
	[member_two] = "member two",
	[member_one] = "member one",
};

struct {
	int quot, rem;
} c99_6_7_8_p34_example10 = { .quot = 2, .rem = -1 };

struct { int a[3], b; } c99_6_7_8_p35_example11[] =
	{ [0].a = {1}, [1].a[0] = 2 };

int c99_6_7_8_p36_example12a[16] = {
	1, 3, 5, 7, 9, [16-5] = 8, 6, 4, 2, 0
};

int c99_6_7_8_p36_example12b[8] = {
	1, 3, 5, 7, 9, [8-5] = 8, 6, 4, 2, 0
};

union {
	int first_member;
	void *second_member;
	unsigned char any_member;
} c99_6_7_8_p38_example13 = { .any_member = 42 };


/*
 * During initialization of an object of type array of unknown size, the type
 * information on the symbol is updated in-place.  Ensure that this happens on
 * a copy of the type.
 *
 * C99 6.7.8p31 example 7
 */
void
ensure_array_type_is_not_modified_during_initialization(void)
{
	typedef int array_of_unknown_size[];

	array_of_unknown_size a1 = { 1, 2, 3};

	switch (4) {
	case sizeof(array_of_unknown_size):
	/* expect+1: error: duplicate case in switch: 0 [199] */
	case 0:
	case 3:
	case 4:
	case 12:
		break;
	}

	/* expect+1: error: negative array dimension (-12) [20] */
	typedef int reveal_sizeof_a1[-(int)(sizeof(a1))];
}

struct point unknown_member_name_beginning = {
	 /* expect+1: error: type 'struct point' does not have member 'r' [101] */
	.r = 5,
	.x = 4,
	.y = 3,
};

struct point unknown_member_name_middle = {
	.x = 4,
	/* expect+1: error: type 'struct point' does not have member 'r' [101] */
	.r = 5,
	.y = 3,
};

struct point unknown_member_name_end = {
	.x = 4,
	.y = 3,
	/* expect+1: error: type 'struct point' does not have member 'r' [101] */
	.r = 5,
};

union value {
	int int_value;
	void *pointer_value;
};

union value unknown_union_member_name_first = {
	/* expect+1: error: type 'union value' does not have member 'unknown_value' [101] */
	.unknown_value = 4,
	.int_value = 3,
};

union value unknown_union_member_name_second = {
	.int_value = 3,
	/* expect+1: error: type 'union value' does not have member 'unknown_value' [101] */
	.unknown_value = 4,
};

struct point subscript_designator_on_struct = {
	/* expect+1: error: syntax error 'designator '[...]' is only for arrays' [249] */
	[0] = 3,
};

struct point unknown_member_on_struct = {
	/* expect+1: error: type 'struct point' does not have member 'member' [101] */
	.member[0][0].member = 4,
};

struct point unknown_member_on_scalar = {
	/* expect+1: error: syntax error 'scalar type cannot use designator' [249] */
	.x.y.z = 5,
};

struct {
	int:16;
	/* expect+2: warning: structure has no named members [65] */
	/* expect+1: error: cannot initialize struct/union with no named member [179] */
} struct_with_only_unnamed_members = {
	123,
};

union {
	int:16;
	/* expect+2: warning: union has no named members [65] */
	/* expect+1: error: cannot initialize struct/union with no named member [179] */
} union_with_only_unnamed_members = {
	123,
};

int designator_for_scalar = {
	 /* expect+1: error: syntax error 'scalar type cannot use designator' [249] */
	.value = 3,
};

struct point member_designator_for_scalar_in_struct = {
	/* expect+1: error: syntax error 'scalar type cannot use designator' [249] */
	{ .x = 3 },
};
struct point subscript_designator_for_scalar_in_struct = {
	/* expect+1: error: syntax error 'designator '[...]' is only for arrays' [249] */
	{ [1] = 4 },
};


/* Seen in pcidevs_data.h, variable 'pci_words'. */
const char string_initialized_with_braced_literal[] = {
	"initializer",
};
