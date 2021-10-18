/* $NetBSD: token_type_def.c,v 1.1 2021/10/18 22:30:34 rillig Exp $ */
/* $FreeBSD$ */

/*
 * Tests for the keyword 'typedef'.
 */

/*
 * Contrary to declarations, type definitions are not affected by the option
 * '-di'.
 */
#indent input
typedef int number;
#indent end

#indent run-equals-input
