/*	$NetBSD: d_c99_bool.c,v 1.1 2021/01/10 11:24:42 rillig Exp $	*/
# 3 "d_bool.c"

/*
 * C99 6.3.1.2 says: "When any scalar value is converted to _Bool, the result
 * is 0 if the value compares equal to 0; otherwise the result is 1."
 *
 * This is different from the other integer types, which get truncated or
 * invoke undefined behavior.
 */

/* Below, the wrong assertions produce warning 20. */

int int_0_converts_to_false[(_Bool)0 ? -1 : 1];
int int_0_converts_to_true_[(_Bool)0 ? 1 : -1];

int int_1_converts_to_false[(_Bool)1 ? -1 : 1];
int int_1_converts_to_true_[(_Bool)1 ? 1 : -1];

int int_2_converts_to_false[(_Bool)2 ? -1 : 1];
int int_2_converts_to_true_[(_Bool)2 ? 1 : -1];

int int_256_converts_to_false[(_Bool)256 ? -1 : 1]; // FIXME
int int_256_converts_to_true_[(_Bool)256 ? 1 : -1]; // FIXME

int null_pointer_converts_to_false[(_Bool)(void *)0 ? -1 : 1];
int null_pointer_converts_to_true_[(_Bool)(void *)0 ? 1 : -1];

int nonnull_pointer_converts_to_false[(_Bool)"not null" ? -1 : 1]; // FIXME 133
int nonnull_pointer_converts_to_true_[(_Bool)"not null" ? 1 : -1]; // FIXME 133

int double_minus_1_0_converts_to_false[(_Bool)-1.0 ? -1 : 1]; // FIXME 119
int double_minus_1_0_converts_to_true_[(_Bool)-1.0 ? 1 : -1]; // FIXME 20, 119

int double_minus_0_5_converts_to_false[(_Bool)-0.5 ? -1 : 1]; // FIXME 119
int double_minus_0_5_converts_to_true_[(_Bool)-0.5 ? 1 : -1]; // FIXME 20, 119

int double_minus_0_0_converts_to_false[(_Bool)-0.0 ? -1 : 1];
int double_minus_0_0_converts_to_true_[(_Bool)-0.0 ? 1 : -1];

int double_0_0_converts_to_false[(_Bool)0.0 ? -1 : 1];
int double_0_0_converts_to_true_[(_Bool)0.0 ? 1 : -1];

/* The C99 rationale explains in 6.3.1.2 why (_Bool)0.5 is true. */
int double_0_5_converts_to_false[(_Bool)0.5 ? -1 : 1]; // FIXME 20
int double_0_5_converts_to_true_[(_Bool)0.5 ? 1 : -1]; // FIXME 20

int double_1_0_converts_to_false[(_Bool)1.0 ? -1 : 1];
int double_1_0_converts_to_true_[(_Bool)1.0 ? 1 : -1];
