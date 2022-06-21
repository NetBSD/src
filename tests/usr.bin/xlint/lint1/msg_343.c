/*	$NetBSD: msg_343.c,v 1.8 2022/06/21 21:18:30 rillig Exp $	*/
# 3 "msg_343.c"

/* Test for message: static array size is a C11 extension [343] */

/* lint1-flags: -Sw */

void takes_int_pointer(int []);
void takes_int_pointer_with_ignored_size(int [3]);
/* expect+1: error: static array size is a C11 extension [343] */
void takes_int_array(int[static 3]);
/* expect+1: error: syntax error '3' [249] */
void takes_volatile_int_array(int[volatile 3]);

int
returns_int_pointer(int a[])
{
	return a[0];
}

int
returns_int_pointer_with_ignored_size(int a[3])
{
	return a[0];
}

int
/* expect+1: error: static array size is a C11 extension [343] */
returns_int_array(int a[static 3])
{
	return a[0];
}

int
/* expect+1: error: syntax error '3' [249] */
returns_volatile_int_array(int a[volatile 3])
{
	/* expect+2: error: cannot dereference non-pointer type 'int' [96] */
	/* expect+1: ... expects to return value [214] */
	return a[0];
}

/*
 * This triggers the "Bad attribute", but for some reason, that custom error
 * message does not make it into the actual diagnostic.
 */
/* expect+2: error: syntax error ']' [249] */
/* expect+1: error: static array size is a C11 extension [343] */
void invalid_storage_class(int a[const typedef 3]);
