/* $NetBSD: lsym_lbrace.c,v 1.6 2022/04/24 09:04:12 rillig Exp $ */

/*
 * Tests for the token lsym_lbrace, which represents a '{' in these contexts:
 *
 * In an initializer, '{' starts an inner group of initializers, usually to
 * initialize a nested struct, union or array.
 *
 * In a function body, '{' starts a block.
 *
 * In an expression, '(type){' starts a compound literal that is typically
 * used in an assignment to a struct or array.
 *
 * In macro arguments, a '{' is an ordinary character, it does not need to be
 * balanced.  This is in contrast to '(', which must be balanced with ')'.
 *
 * TODO: try to split this token into lsym_lbrace_block and lsym_lbrace_init.
 */

/* Brace level in an initializer */
//indent input
void
function(void)
{
	struct person	p = {
		.name = "Name",
		.age = {{{35}}},	/* C11 6.7.9 allows this. */
	};
}
//indent end

//indent run-equals-input


/* Begin of a block of statements */
//indent input
void function(void) {{{ body(); }}}
//indent end

//indent run
void
function(void)
/* $ FIXME: Each '{' must be properly indented. */
{{{
			body();
}
}
}
//indent end


/* Compound literal */
//indent input
struct point
origin(void)
{
	return (struct point){
		.x = 0,
		.y = 0,
	};
}
//indent end

//indent run
struct point
origin(void)
{
	return (struct point){
		.x = 0,
/* $ FIXME: All initializers must be indented to the same level. */
			.y = 0,
	};
}
//indent end
