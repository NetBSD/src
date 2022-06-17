/*	$NetBSD: expr_precedence.c,v 1.9 2022/06/17 18:54:53 rillig Exp $	*/
# 3 "expr_precedence.c"

/*
 * Tests for the precedence among operators.
 */

int var;

/*
 * An initializer needs an assignment-expression; the comma must be
 * interpreted as a separator, not an operator.
 */
/* expect+1: error: syntax error '4' [249] */
int init_error = 3, 4;

/* expect+1: error: non-constant initializer [177] */
int init_syntactically_ok = var = 1 ? 2 : 3;

/*
 * The arguments of __attribute__ must be constant-expression, as assignments
 * don't make sense at that point.
 */
void __attribute__((format(printf,
    /*
     * Inside of __attribute__((...)), symbol lookup works differently.  For
     * example, 'printf' is a keyword, and since all arguments to
     * __attribute__ are constant expressions, looking up global variables
     * would not make sense.  Therefore, 'var' is undefined.
     *
     * See lex.c, function 'search', keyword 'in_gcc_attribute'.
     */
    /* expect+2: error: 'var' undefined [99] */
    /* expect+1: error: syntax error '=' [249] */
    var = 1,
    /* Syntactically ok, must be a constant expression though. */
    var > 0 ? 2 : 1)))
my_printf(const char *, ...);

void
assignment_associativity(int arg)
{
	int left, right;

	/*
	 * Assignments are right-associative.  If they were left-associative,
	 * the result of (left = right) would be an rvalue, resulting in this
	 * error message: 'left operand of '=' must be lvalue [114]'.
	 */
	left = right = arg;

	left = arg;
}

void
conditional_associativity(_Bool cond1, _Bool cond2, int a, int b, int c)
{
	/* The then-expression can be an arbitrary expression. */
	var = cond1 ? cond2 ? a : b : c;
	var = cond1 ? (cond2 ? a : b) : c;

	/* The then-expression can even be a comma-expression. */
	var = cond1 ? cond2 ? a, b : (b, a) : c;

	var = cond1 ? a : cond2 ? b : c;
	/*
	 * In almost all programming languages, '?:' is right-associative,
	 * which allows for easy chaining.
	 */
	var = cond1 ? a : (cond2 ? b : c);
	/*
	 * In PHP, '?:' is left-associative, which is rather surprising and
	 * requires more parentheses to get the desired effect.
	 */
	var = (cond1 ? a : cond2) ? b : c;
}
