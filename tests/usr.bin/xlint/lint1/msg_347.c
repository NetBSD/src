/*	$NetBSD: msg_347.c,v 1.1 2021/09/12 16:28:45 rillig Exp $	*/
# 3 "msg_347.c"

// Test for message: redeclaration of '%s' with type '%s', expected '%s' [347]

/* lint1-extra-flags: -r */

/*
 * Message 27 already covers redeclarations, but it doesn't include enough
 * details to make any sense of it.
 */

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
/* FIXME: the type of the second parameter is not 'int' */
/* expect+1: previous declaration of function_parameter [260] */
void function_parameter(void *, double *(void *, int));
/* expect+1: error: redeclaration of 'function_parameter' with type 'function(pointer to void, pointer to function(pointer to void, int) returning pointer to double) returning void', expected 'function(pointer to void, int) returning void' [347] */
void function_parameter(void *fs, double *func(void *, int));
