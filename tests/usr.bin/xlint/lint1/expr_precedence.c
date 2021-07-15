/*	$NetBSD: expr_precedence.c,v 1.1 2021/07/15 17:09:08 rillig Exp $	*/
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
    /* expect+2: error: 'var' undefined [99] */ /* XXX: why? */
    /* XXX: allow only constant-expression, not assignment-expression */
    var = 1,
    /* Syntactically ok, must be a constant expression though. */
    /* expect+1: error: 'var' undefined [99] */
    var > 0 ? 2 : 1)))
my_printf(const char *, ...);
