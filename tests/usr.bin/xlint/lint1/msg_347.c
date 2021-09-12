/*	$NetBSD: msg_347.c,v 1.2 2021/09/12 17:30:53 rillig Exp $	*/
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


/* expect+1: warning: struct last_arg never defined [233] */
struct last_arg;
/*
 * FIXME: The following error is completely wrong.
 * There is no argument that has 'struct last_arg', there are only pointers
 * to it.
 */
/* expect+2: error: '<unnamed>' has incomplete type 'incomplete struct last_arg' [31] */
/* expect+1: previous declaration of last_arg_struct [260] */
void last_arg_struct(double, double *(struct last_arg *));
/* expect+1: error: redeclaration of 'last_arg_struct' with type 'function(double, pointer to function(pointer to incomplete struct last_arg) returning pointer to double) returning void', expected 'function(double, incomplete struct last_arg) returning void' [347] */
void last_arg_struct(double d, double *fn(struct last_arg *));
