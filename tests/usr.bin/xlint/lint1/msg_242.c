/*	$NetBSD: msg_242.c,v 1.6 2022/06/22 19:23:18 rillig Exp $	*/
# 3 "msg_242.c"

// Test for message: combination of '%s' and '%s', op '%s' [242]

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

	/* expect+1: warning: combination of 'enum E' and 'int', op '=' [242] */
	e3 = i;
	/* expect+1: warning: combination of 'int' and 'enum E', op '=' [242] */
	i2 = e;

	sink_enum(e2);
	sink_enum(e3);
	sink_int(i2);
	sink_int(i3);
}


/*
 * In C, the only ways to create named compile-time integer constants are
 * preprocessor macros or enum constants. All other expressions do not count
 * as constant expressions, even if they are declared 'static const' or
 * 'const'.
 */
unsigned
unnamed_enum(void)
{
	enum {
		compile_time_constant = 2
	};

	unsigned i = 3;

	/* expect+3: warning: dubious operation on enum, op '*' [241] */
	/* FIXME: Combining 'unsigned int' with 'unsigned int' is OK. */
	/* expect+1: warning: combination of 'unsigned int' and 'unsigned int', op '=' [242] */
	i = compile_time_constant * i;
	return i;
}
