#	$NetBSD: Makefile,v 1.12 1997/08/01 14:51:44 jtk Exp $
#	@(#)Makefile	8.5 (Berkeley) 3/27/94

WARNS=	1
PROG=	mount
MAN=	mount.8
# We do NOT install the getmntopts.3 man page.
# getmntopts.c in this directory isn't used by mount anymore, but it
# is used by other mount programs which use .PATH directives to use
# the version here.

.include <bsd.prog.mk>
