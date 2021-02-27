/*	$NetBSD: msg_278.c,v 1.3 2021/02/27 18:01:29 rillig Exp $	*/
# 3 "msg_278.c"

// Test for message: combination of '%s' and '%s', arg #%d [278]

/* lint1-extra-flags: -e */

enum E {
	E1
};

void sink_enum(enum E);
void sink_int(int);

void
example(enum E e, int i)
{
	sink_enum(e);
	sink_enum(i);		/* expect: 278 */

	sink_int(e);		/* expect: 278 */
	sink_int(i);
}
