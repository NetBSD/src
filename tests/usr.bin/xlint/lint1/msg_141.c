/*	$NetBSD: msg_141.c,v 1.6 2023/03/28 14:44:34 rillig Exp $	*/
# 3 "msg_141.c"

// Test for message: integer overflow detected, op '%s' [141]

/* lint1-extra-flags: -h -X 351 */

/*
 * Before tree.c 1.347 from 2021-08-23, lint wrongly warned about integer
 * overflow in '-'.
 */
int signed_int_max = (1u << 31) - 1;

/*
 * Before tree.c 1.347 from 2021-08-23, lint wrongly warned about integer
 * overflow in '-'.
 */
unsigned int unsigned_int_max = (1u << 31) - 1;

/* expect+1: warning: integer overflow detected, op '+' [141] */
int int_overflow = (1 << 30) + (1 << 30);

/* expect+2: warning: integer overflow detected, op '+' [141] */
/* expect+1: warning: initialization of unsigned with negative constant [221] */
unsigned int intermediate_overflow = (1 << 30) + (1 << 30);

unsigned int no_overflow = (1U << 30) + (1 << 30);

/* expect+1: warning: integer overflow detected, op '-' [141] */
unsigned int unsigned_int_min = 0u - (1u << 31);

/* expect+1: warning: integer overflow detected, op '-' [141] */
unsigned int unsigned_int_min_unary = -(1u << 31);
