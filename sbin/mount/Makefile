#	$NetBSD: Makefile,v 1.27 2003/01/19 12:37:41 tsutsui Exp $
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

.include <bsd.prog.mk>
