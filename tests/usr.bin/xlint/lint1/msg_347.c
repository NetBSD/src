/*	$NetBSD: msg_347.c,v 1.7 2023/08/02 21:11:35 rillig Exp $	*/
# 3 "msg_347.c"

// Test for message: redeclaration of '%s' with type '%s', expected '%s' [347]

/* lint1-extra-flags: -r -X 351 */

/*
 * Message 27 already covers redeclarations, but it doesn't include enough
 * details to make any sense of it.
 */

/*
 * Before 2023-08-02, lint did not interpret unnamed function parameters with
 * complicated types correctly. The named counterparts were no problem, though.
 */

void function_parameter(void *, double *(void *, int));
void function_parameter(void *fs, double *fn(void *, int));


/* expect+1: warning: struct 'last_arg' never defined [233] */
struct last_arg;
void last_arg_struct(double, double *(struct last_arg *));
void last_arg_struct(double d, double *fn(struct last_arg *));


struct last_param {
	int member;
};

void last_param(double, double *(struct last_param));
void last_param(double d, double *fn(struct last_param));


/* expect+1: previous declaration of 'mismatch' [260] */
void mismatch(double, double *(struct last_param));
/* expect+1: error: redeclaration of 'mismatch' with type 'function(double, pointer to function(struct last_param) returning pointer to float) returning void', expected 'function(double, pointer to function(struct last_param) returning pointer to double) returning void' [347] */
void mismatch(double d, float *fn(struct last_param));
