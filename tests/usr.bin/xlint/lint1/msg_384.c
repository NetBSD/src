/*	$NetBSD: msg_384.c,v 1.1 2024/12/01 18:37:54 rillig Exp $	*/
# 3 "msg_384.c"

// Test for message: function definition with identifier list is obsolete in C23 [384]

/* lint1-extra-flags: -X 351 */

/*
 * In traditional C, defining a function by listing its parameter names first,
 * followed by declarations, was usual. This practice has been obsoleted in
 * favor of defining the parameter types right in the declarator.
 */

static inline int
/* expect+1: warning: function definition with identifier list is obsolete in C23 [384] */
function_with_identifier_list(a, b)
	int a, b;
{
	return a + b;
}

static inline int
function_with_prototype(int a, int b)
{
	return a + b;
}
