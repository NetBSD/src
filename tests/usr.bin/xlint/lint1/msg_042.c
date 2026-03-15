/*	$NetBSD: msg_042.c,v 1.4 2026/03/15 18:32:16 rillig Exp $	*/
# 3 "msg_042.c"

/* Test for message: forward reference to enum type [42] */

/*
 * When an enum type is declared without specifying its enum constants, it is
 * not yet known how large the enum type will become in the end.  The compiler
 * may choose an underlying type different from 'int' for it, as an extension
 * to standard C90, C99, C11 and C17.
 *
 * In C23, a forward declaration of an enum type without a fixed underlying
 * type is a constraint violation, see C23 6.7.3.3p19.
 */

/* lint1-extra-flags: -p */

/* expect+1: warning: forward reference to enum type [42] */
enum forward;

enum forward {
	defined
};
