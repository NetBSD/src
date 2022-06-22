/*	$NetBSD: msg_225.c,v 1.4 2022/06/22 19:23:18 rillig Exp $	*/
# 3 "msg_225.c"

// Test for message: static function '%s' called but not defined [225]

static void undefined(void);

static void defined_later(void);

void
caller(void)
{
	/* expect+1: error: static function 'undefined' called but not defined [225] */
	undefined();
	defined_later();
}

static void
defined_later(void)
{
}
