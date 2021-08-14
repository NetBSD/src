/*	$NetBSD: msg_242.c,v 1.4 2021/08/14 12:46:24 rillig Exp $	*/
# 3 "msg_242.c"

// Test for message: combination of '%s' and '%s', op %s [242]

/* lint1-extra-flags: -e */

enum E {
	E1
};

void sink_enum(enum E);
void sink_int(int);

void
example(enum E e, int i)
{
	enum E e2 = e;
	/* expect+1: warning: initialization of 'enum E' with 'int' [277] */
	enum E e3 = i;
	/* expect+1: warning: initialization of 'int' with 'enum E' [277] */
	int i2 = e;
	int i3 = i;

	/* expect+1: warning: combination of 'enum E' and 'int', op = [242] */
	e3 = i;
	/* expect+1: warning: combination of 'int' and 'enum E', op = [242] */
	i2 = e;

	sink_enum(e2);
	sink_enum(e3);
	sink_int(i2);
	sink_int(i3);
}
