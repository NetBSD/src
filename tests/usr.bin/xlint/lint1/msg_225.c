/*	$NetBSD: msg_225.c,v 1.3 2022/06/16 21:24:41 rillig Exp $	*/
# 3 "msg_225.c"

// Test for message: static function called but not defined: %s() [225]

static void undefined(void);

static void defined_later(void);

void
caller(void)
{
	/* expect+1: error: static function called but not defined: undefined() [225] */
	undefined();
	defined_later();
}

static void
defined_later(void)
{
}
