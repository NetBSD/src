/*	$NetBSD: msg_279.c,v 1.4 2022/06/16 21:24:41 rillig Exp $	*/
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
	/* expect+1: warning: combination of 'enum E' and 'int' in return [279] */
	return i;
}

int
returning_int(enum E e)
{
	/* expect+1: warning: combination of 'int' and 'enum E' in return [279] */
	return e;
}
