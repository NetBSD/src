#	$NetBSD: Makefile,v 1.1 2007/04/21 14:21:42 pooka Exp $
#

PROG=	mount_9p
SRCS=	ninepuffs.c ninebuf.c nineproto.c fs.c node.c subr.c
LDADD+= -lpuffs -lutil
WARNS=	4
DBG=-g -O0

MAN=	mount_9p.8

.include <bsd.prog.mk>
