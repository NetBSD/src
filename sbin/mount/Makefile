#	$NetBSD: Makefile,v 1.14.6.1 1999/10/19 20:04:03 fvdl Exp $
#	@(#)Makefile	8.6 (Berkeley) 5/8/95

PROG=	mount
MAN=	mount.8
SRCS=	mount.c vfslist.c
CPPFLAGS+=-I${.CURDIR}/../../sys
# We do NOT install the getmntopts.3 man page.
# getmntopts.c in this directory isn't used by mount anymore, but it
# is used by other mount programs which use .PATH directives to use
# the version here.

.include <bsd.prog.mk>
