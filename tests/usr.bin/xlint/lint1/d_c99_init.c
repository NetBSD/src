/*	$NetBSD: d_c99_init.c,v 1.18 2021/03/28 18:48:32 rillig Exp $	*/
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


// C99 6.7.8p11 says "optionally enclosed in braces".  There is no limitation
// on the number of brace pairs.
int scalar_without_braces = 3;
int scalar_with_optional_braces = { 3 };
int scalar_with_too_many_braces = {{ 3 }};
int scalar_with_too_many_initializers = { 3, 5 };	/* expect: 174 */


// See init_using_expr, 'handing over to ASSIGN'.
void
struct_initialization_via_assignment(any arg)
{
	any local = arg;
	use(&local);
}


// See init_using_expr, initstack_string.
char static_duration[] = "static duration";
signed char static_duration_signed[] = "static duration";
unsigned char static_duration_unsigned[] = "static duration";
int static_duration_wchar[] = L"static duration";

// See init_using_expr.
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
	any local = { arg };	/* expect: 185 */
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
	444,			/* expect: too many array initializers */
};

// See initstack_push, 'extending array of unknown size'.
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
	// FIXME: assertion failure '== ARRAY'
	// 5,
	.x = 3,
};

int array_with_designator[] = {
	111,
	.member = 222,		/* expect: 249 */
	333,
};

/*
 * C99 6.7.8p11 says that the initializer of a scalar can be "optionally
 * enclosed in braces".  It does not explicitly set an upper limit on the
 * number of braces.  It also doesn't restrict the term "initializer" to only
 * mean the "outermost initializer".  Both GCC 10 and Clang 8 already warn
 * about this, so there is no extra work for lint to do.
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
		 * This test case demonstrates that in
		 * extend_if_array_of_unknown_size, setcomplete is called too
		 * early.
		 */
		sizeof points,
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
	// FIXME: assertion "istk->i_type != NULL" failed in initstack_push
	//.pentagons[0].points[4].x = 1,
	.points[0][0][0] = { 0, 0 },
	.points[2][4][1] = {301, 302 },
	/* TODO: expect+1: array index 3 must be between 0 and 2 */
	.points[3][0][0] = {3001, 3002 },
	/* TODO: expect+1: array index 5 must be between 0 and 4 */
	.points[0][5][0] = {501, 502 },
	/* TODO: expect+1: array index 2 must be between 0 and 1 */
	.points[0][0][2] = {21, 22 },
};

struct ends_with_unnamed_bit_field {
	int member;
	int : 0;
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
	/* expect+1: too many array initializers */
	'\n',
};
