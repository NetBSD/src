#	$NetBSD: Makefile,v 1.6 2020/08/30 21:12:45 christos Exp $
#

.include <bsd.init.mk>

PROG=	mount_9p
SRCS=	ninepuffs.c ninebuf.c nineproto.c fs.c node.c subr.c
LDADD+= -lpuffs -lutil

MAN=	mount_9p.8

CPPFLAGS += ${${USE_INET6} != "no":? -DINET6 :}

.include <bsd.prog.mk>
