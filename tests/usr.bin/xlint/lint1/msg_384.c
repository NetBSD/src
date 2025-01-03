/*	$NetBSD: msg_384.c,v 1.2 2025/01/03 03:14:47 rillig Exp $	*/
# 3 "msg_384.c"

// Test for message: function definition for '%s' with identifier list is obsolete in C23 [384]

/* lint1-extra-flags: -X 351 */

/*
 * In traditional C, defining a function by listing its parameter names first,
 * followed by declarations, was usual. This practice has been obsoleted in
 * favor of defining the parameter types right in the declarator.
 */

static inline int
/* expect+1: warning: function definition for 'function_with_identifier_list' with identifier list is obsolete in C23 [384] */
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
