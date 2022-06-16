/*	$NetBSD: msg_177.c,v 1.3 2022/06/16 16:58:36 rillig Exp $	*/
# 3 "msg_177.c"

// Test for message: non-constant initializer [177]

extern int function(void);

static const int not_a_constant = 13;

/* expect+1: error: non-constant initializer [177] */
const int var = not_a_constant;

/* expect+1: error: non-constant initializer [177] */
const int calling_function = function();
