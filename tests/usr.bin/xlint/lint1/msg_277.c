/*	$NetBSD: msg_277.c,v 1.8 2023/07/07 06:03:31 rillig Exp $	*/
# 3 "msg_277.c"

// Test for message: initialization of '%s' with '%s' [277]

/* lint1-extra-flags: -e -X 351 */

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
	enum E e3 = { i };
	/* expect+1: warning: initialization of 'int' with 'enum E' [277] */
	int i2 = { e };
	int i3 = i;

	sink_enum(e2);
	sink_enum(e3);
	sink_int(i2);
	sink_int(i3);

	/* expect+1: warning: 'init_0' set but not used in function 'example' [191] */
	enum E init_0 = 0;
	/* expect+2: warning: 'init_1' set but not used in function 'example' [191] */
	/* expect+1: warning: initialization of 'enum E' with 'int' [277] */
	enum E init_1 = 1;
}
