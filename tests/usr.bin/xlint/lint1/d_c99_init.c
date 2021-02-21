/*	$NetBSD: d_c99_init.c,v 1.4 2021/02/21 12:49:05 rillig Exp $	*/
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
void
initialization_by_string(void)
{
	any local = { "hello" };
	use(&local);
}

void
initialization_with_redundant_braces(any arg)
{
	any local = { arg };	/* expect: 185 */
	// FIXME: message 185 needs to be reworded to "cannot initialize '%s' from '%s'".
	use(&arg);
}
