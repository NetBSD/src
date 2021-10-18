/* $NetBSD: token_question.c,v 1.1 2021/10/18 22:30:34 rillig Exp $ */
/* $FreeBSD$ */

/*
 * Tests for the '?:' operator.
 */

#indent input
int var = cond ? 1 : 0;

int multi = cond ? 1 : cond ? 2 : cond ? 3 : 4;
#indent end

#indent run-equals-input -di0
