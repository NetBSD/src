/*	$NetBSD: msg_347.c,v 1.4 2022/06/11 11:52:13 rillig Exp $	*/
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
/* expect+1: previous declaration of 'function_parameter' [260] */
void function_parameter(void *, double *(void *, int));
/* expect+1: error: redeclaration of 'function_parameter' with type 'function(pointer to void, pointer to function(pointer to void, int) returning pointer to double) returning void', expected 'function(pointer to void, int) returning void' [347] */
void function_parameter(void *fs, double *func(void *, int));


/* expect+1: warning: struct 'last_arg' never defined [233] */
struct last_arg;
/*
 * FIXME: The following error is completely wrong.
 *  There is no argument that has 'struct last_arg', there are only pointers
 *  to it.
 */
/* expect+2: error: '<unnamed>' has incomplete type 'incomplete struct last_arg' [31] */
/* expect+1: previous declaration of 'last_arg_struct' [260] */
void last_arg_struct(double, double *(struct last_arg *));
/* expect+1: error: redeclaration of 'last_arg_struct' with type 'function(double, pointer to function(pointer to incomplete struct last_arg) returning pointer to double) returning void', expected 'function(double, incomplete struct last_arg) returning void' [347] */
void last_arg_struct(double d, double *fn(struct last_arg *));


struct last_param {
	int member;
};

/* expect+1: previous declaration of 'last_param' [260] */
void last_param(double, double *(struct last_param));

/*
 * FIXME: The type of last_param is completely wrong.  The second parameter
 *  must be a function, not a struct.
 */
/* expect+1: error: cannot initialize 'double' from 'pointer to function(double, struct last_param) returning void' [185] */
double reveal_type_of_last_param_abstract = last_param;

/* expect+1: error: redeclaration of 'last_param' with type 'function(double, pointer to function(struct last_param) returning pointer to double) returning void', expected 'function(double, struct last_param) returning void' [347] */
void last_param(double d, double *fn(struct last_param));

/* expect+1: error: cannot initialize 'double' from 'pointer to function(double, pointer to function(struct last_param) returning pointer to double) returning void' [185] */
double reveal_type_of_last_param_named = last_param;
