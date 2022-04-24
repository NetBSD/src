/* $NetBSD: token_type_def.c,v 1.3 2022/04/24 09:04:12 rillig Exp $ */

/*
 * Tests for the keyword 'typedef'.
 */

/*
 * Contrary to declarations, type definitions are not affected by the option
 * '-di'.
 */
//indent input
typedef int number;
//indent end

//indent run-equals-input
