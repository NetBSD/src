#	$NetBSD: Makefile,v 1.14 1997/09/16 12:22:39 lukem Exp $
#	@(#)Makefile	8.6 (Berkeley) 5/8/95

PROG=	mount
MAN=	mount.8
SRCS=	mount.c vfslist.c
# We do NOT install the getmntopts.3 man page.
# getmntopts.c in this directory isn't used by mount anymore, but it
# is used by other mount programs which use .PATH directives to use
# the version here.

.include <bsd.prog.mk>
