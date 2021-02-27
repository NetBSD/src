/*	$NetBSD: msg_279.c,v 1.3 2021/02/27 18:01:29 rillig Exp $	*/
# 3 "msg_279.c"

// Test for message: combination of '%s' and '%s' in return [279]

/* lint1-extra-flags: -e */

enum E {
	E1
};

void sink_enum(enum E);
void sink_int(int);

enum E
returning_enum(int i)
{
	return i;		/* expect: 279 */
}

int
returning_int(enum E e)
{
	return e;		/* expect: 279 */
}
