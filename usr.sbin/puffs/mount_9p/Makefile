#	$NetBSD: Makefile,v 1.2 2007/07/18 22:38:59 pooka Exp $
#

PROG=	mount_9p
SRCS=	ninepuffs.c ninebuf.c nineproto.c fs.c node.c subr.c
LDADD+= -lpuffs -lutil
WARNS=	4

MAN=	mount_9p.8

.include <bsd.prog.mk>
