/* $NetBSD: token_form_feed.c,v 1.1 2021/10/18 22:30:34 rillig Exp $ */
/* $FreeBSD$ */

/*
 * Tests for form feeds, which is a control character (C99 5.2.1p3).
 */

#indent input
void function_1(void);

void function_2(void);
#indent end

#indent run-equals-input -di0
