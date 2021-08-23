/*	$NetBSD: msg_141.c,v 1.3 2021/08/23 05:52:04 rillig Exp $	*/
# 3 "msg_141.c"

// Test for message: integer overflow detected, op %s [141]

/* lint1-extra-flags: -h */

/* FIXME */
/* expect+1: warning: integer overflow detected, op - [141] */
int signed_int_max = (1u << 31) - 1;

/* FIXME */
/* expect+1: warning: integer overflow detected, op - [141] */
unsigned int unsigned_int_max = (1u << 31) - 1;

/* expect+1: warning: integer overflow detected, op + [141] */
int int_overflow = (1 << 30) + (1 << 30);

/* expect+2: warning: integer overflow detected, op + [141] */
/* expect+1: warning: initialization of unsigned with negative constant [221] */
unsigned int intermediate_overflow = (1 << 30) + (1 << 30);

unsigned int no_overflow = (1U << 30) + (1 << 30);
