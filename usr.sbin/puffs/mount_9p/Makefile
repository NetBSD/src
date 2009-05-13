#	$NetBSD: Makefile,v 1.2.20.1 2009/05/13 19:20:33 jym Exp $
#

PROG=	mount_9p
SRCS=	ninepuffs.c ninebuf.c nineproto.c fs.c node.c subr.c
LDADD+= -lpuffs -lutil

MAN=	mount_9p.8

.include <bsd.prog.mk>
