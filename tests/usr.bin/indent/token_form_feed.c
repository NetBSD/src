/* $NetBSD: token_form_feed.c,v 1.2 2021/11/07 18:38:34 rillig Exp $ */
/* $FreeBSD$ */

/*
 * Tests for form feeds, which is a control character (C99 5.2.1p3).
 */

#indent input
void function_1(void);

void function_2(void);
#indent end

#indent run -di0
void function_1(void);

/* $ XXX: The form feed is not preserved. */
/* $ XXX: Why 2 empty lines? */

void function_2(void);
#indent end
