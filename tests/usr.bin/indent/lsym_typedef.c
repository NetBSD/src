/* $NetBSD: lsym_typedef.c,v 1.2 2022/02/12 13:38:29 rillig Exp $ */
/* $FreeBSD$ */

/*
 * Tests for the token lsym_typedef, which represents the keyword 'typedef'
 * for giving a type an additional name.
 */

/*
 * Since 2019-04-04, indent places all enum constants except the first in the
 * wrong column, but only if the enum declaration follows a 'typedef'.
 *
 * https://gnats.netbsd.org/55453
 */
#indent input
typedef enum {
    TC1,
    TC2
} T;

enum {
    EC1,
    EC2
} E;
#indent end

/* FIXME: TC2 is indented too far. */
#indent run -ci4 -i4
typedef enum {
    TC1,
	TC2
} T;

enum {
    EC1,
    EC2
}		E;
#indent end

/* FIXME: TC2 is indented too far. */
#indent run -ci2
typedef enum {
	TC1,
	  TC2
} T;

enum {
	EC1,
	EC2
}		E;
#indent end
