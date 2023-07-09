/*	$NetBSD: msg_141.c,v 1.7 2023/07/09 11:01:27 rillig Exp $	*/
# 3 "msg_141.c"

// Test for message: operator '%s' produces integer overflow [141]

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

/* expect+1: warning: operator '+' produces integer overflow [141] */
int int_overflow = (1 << 30) + (1 << 30);

/* expect+2: warning: operator '+' produces integer overflow [141] */
/* expect+1: warning: initialization of unsigned with negative constant [221] */
unsigned int intermediate_overflow = (1 << 30) + (1 << 30);

unsigned int no_overflow = (1U << 30) + (1 << 30);

/* expect+1: warning: operator '-' produces integer overflow [141] */
unsigned int unsigned_int_min = 0u - (1u << 31);

/* expect+1: warning: operator '-' produces integer overflow [141] */
unsigned int unsigned_int_min_unary = -(1u << 31);
