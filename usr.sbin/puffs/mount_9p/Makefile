#	$NetBSD: Makefile,v 1.5 2020/06/19 01:19:50 uwe Exp $
#

PROG=	mount_9p
SRCS=	ninepuffs.c ninebuf.c nineproto.c fs.c node.c subr.c
LDADD+= -lpuffs -lutil

MAN=	mount_9p.8

CPPFLAGS += ${${USE_INET6} != "no":? -DINET6 :}

.include <bsd.prog.mk>
