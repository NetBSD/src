#	$NetBSD: Makefile,v 1.26 2003/01/19 10:49:12 jdolecek Exp $
#	@(#)Makefile	8.6 (Berkeley) 5/8/95

.include <bsd.own.mk>

PROG=	mount
MAN=	mount.8
SRCS=	mount.c vfslist.c
# We do NOT install the getmntopts.3 man page.
# getmntopts.c in this directory is used by different mount programs which
# use .PATH directives to get it.

.if defined(RESCUEDIR)
CPPFLAGS+= -D_PATH_RESCUE=\"${RESCUEDIR}\"
.endif

DPADD+=${LIBUTIL}
LDADD+=-lutil

.include <bsd.prog.mk>
