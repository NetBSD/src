/*	$NetBSD: expr_precedence.c,v 1.3 2021/07/15 17:48:10 rillig Exp $	*/
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
     * See lex.c, function 'search', keyword 'attron'.
     */
    /* expect+2: error: 'var' undefined [99] */
    /* expect+1: syntax error '=' [249] */
    var = 1,
    /* Syntactically ok, must be a constant expression though. */
    var > 0 ? 2 : 1)))
my_printf(const char *, ...);
