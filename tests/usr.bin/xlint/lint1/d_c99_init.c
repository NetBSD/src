/*	$NetBSD: d_c99_init.c,v 1.9 2021/03/19 01:02:52 rillig Exp $	*/
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


// C99 6.7.8p11 says "optionally enclosed in braces".  The intended
// interpretation is "optionally enclosed in a single pair of braces".
int scalar_without_braces = 3;
int scalar_with_optional_braces = { 3 };
int scalar_with_too_many_braces = {{ 3 }};		/* expect: 176 */
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

// See init_using_expr.
void
initialization_by_braced_string(void)
{
	any local = { "hello" };
	use(&local);
}

void
initialization_with_redundant_braces(any arg)
{
	any local = { arg };	/* expect: 185 */
	// TODO: message 185 needs to be reworded to "cannot initialize '%s' from '%s'".
	use(&arg);
}

// Some of the following examples are mentioned in init.c.

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

// See d_struct_init_nested.c for a more complicated example.
