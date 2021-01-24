/*	$NetBSD: msg_225.c,v 1.2 2021/01/24 17:55:41 rillig Exp $	*/
# 3 "msg_225.c"

// Test for message: static function called but not defined: %s() [225]

static void undefined(void);

static void defined_later(void);

void
caller(void)
{
	undefined();		/* expect: 225 */
	defined_later();
}

static void
defined_later(void)
{
}
