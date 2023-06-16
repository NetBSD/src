/* $NetBSD: lsym_lbrace.c,v 1.10 2023/06/16 23:19:01 rillig Exp $ */

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
	}, actual_return_value;
}
//indent end

//indent run-equals-input

/* Ensure that the comma is not interpreted as separator for declarators. */
//indent run-equals-input -bc


//indent input
{
	const char *hello = (const char[]){
		'h', 'e', 'l', 'l', 'o',
	}, *world = (const char[]){
		'w', 'o', 'r', 'l', 'd',
	};
}
//indent end

//indent run-equals-input -ldi0

//indent run-equals-input -ldi0 -bc


//indent input
{
	if (cond rparen {
	}
	switch (expr rparen {
	}
}
//indent end

//indent run
{
		if (cond rparen {
		}
		switch (expr rparen {
		}
}
// exit 1
// error: Standard Input:2: Unbalanced parentheses
// error: Standard Input:4: Unbalanced parentheses
//indent end


/*
 * The -bl option does not force initializer braces on separate lines.
 */
//indent input
struct {int member;} var = {1};
//indent end

//indent run -bl
struct
{
	int		member;
}		var = {1};
//indent end


/*
 * A comment in a single-line function definition is not a declaration comment
 * and thus not in column 25.
 */
//indent input
void function(void); /* comment */
void function(void) { /* comment */ }
//indent end

//indent run -di0
void function(void);		/* comment */
void
function(void)
{				/* comment */
}
//indent end

//indent run -di0 -nfbs
void function(void);		/* comment */
void
function(void) {		/* comment */
}
//indent end
