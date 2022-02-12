/* $NetBSD: lsym_typedef.c,v 1.3 2022/02/12 19:56:52 rillig Exp $ */
/* $FreeBSD$ */

/*
 * Tests for the token lsym_typedef, which represents the keyword 'typedef'
 * for giving a type an additional name.
 */

/*
 * Since 2019-04-04 and before lexi.c 1.169 from 2022-02-12, indent placed all
 * enum constants except the first too far to the right, as if it were a
 * statement continuation, but only if the enum declaration followed a
 * 'typedef'.
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
