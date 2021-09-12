/*	$NetBSD: msg_027.c,v 1.4 2021/09/12 16:08:14 rillig Exp $	*/
# 3 "msg_027.c"

// Test for message: redeclaration of %s [27]

extern int identifier(void);

extern double identifier(void);		/* expect: 27 */

/*
 * As of 2021-09-12, lint complains about mismatched types.
 * GCC and Clang accept this.
 *
 * Above:
 *     function(pointer to void, int) returning void
 *
 * Below: function(
 *     pointer to void,
 *     pointer to function(pointer to void, int) returning pointer to double
 * ) returning void
 */
void function_parameter(void *, double *(void *, int));
/* expect+1: error: redeclaration of function_parameter [27] */
void function_parameter(void *fs, double *func(void *, int));
