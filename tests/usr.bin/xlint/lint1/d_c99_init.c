/*	$NetBSD: d_c99_init.c,v 1.6 2021/02/21 14:19:27 rillig Exp $	*/
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
	// FIXME: message 185 needs to be reworded to "cannot initialize '%s' from '%s'".
	use(&arg);
}

// See initstack_push, 'extending array of unknown size'.
const int primes[] = { 2, 3, 5, 7, 9 };
