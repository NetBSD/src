/* $NetBSD: lsym_rbrace.c,v 1.7 2023/06/08 06:47:14 rillig Exp $ */

/*
 * Tests for the token lsym_rbrace, which represents a '}' in these contexts:
 *
 * In an initializer, '}' ends an inner group of initializers, usually to
 * initialize a nested struct, union or array.
 *
 * In a function body, '}' ends a block.
 *
 * In an expression like '(type){...}', '}' ends a compound literal, which is
 * typically used in an assignment to a struct or array.
 *
 * In macro arguments, a '}' is an ordinary character, it does not need to be
 * balanced.  This is in contrast to '(' and ')', which must be balanced.
 *
 * TODO: try to split this token into lsym_rbrace_block and lsym_rbrace_init.
 *
 * See also:
 *	lsym_lbrace.c
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
{
	{
		{
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

//indent run-equals-input


//indent input
{
int numbers[][] = {
{11},
{21},
{31},
};
int numbers[][] = {{11},
{21},
{31},
};
}
//indent end

//indent run -di0
{
	int numbers[][] = {
		{11},
		{21},
		{31},
	};
	int numbers[][] = {{11},
	// $ FIXME: Must be indented.
	{21},
	{31},
	};
}
//indent end
