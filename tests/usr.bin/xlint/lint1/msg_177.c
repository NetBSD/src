/*	$NetBSD: msg_177.c,v 1.2 2021/01/24 16:12:45 rillig Exp $	*/
# 3 "msg_177.c"

// Test for message: non-constant initializer [177]

extern int function(void);

static const int not_a_constant = 13;

const int var = not_a_constant;			/* expect: 177 */

const int calling_function = function();	/* expect: 177 */
