/*	$NetBSD: msg_177.c,v 1.4.2.1 2025/08/02 05:58:16 perseant Exp $	*/
# 3 "msg_177.c"

// Test for message: non-constant initializer [177]

/* lint1-extra-flags: -X 351 */

extern int function(void);

static const int not_a_constant = 13;

/* expect+1: error: non-constant initializer [177] */
const int var = not_a_constant;

/* expect+1: error: non-constant initializer [177] */
const int calling_function = function();

// A compound expression is not a constant expression.
/* expect+1: error: non-constant initializer [177] */
const int compound_expression = (int){ 3 };
