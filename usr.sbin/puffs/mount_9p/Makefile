#	$NetBSD: Makefile,v 1.4 2020/06/13 13:45:06 uwe Exp $
#
.include <bsd.init.mk>		# include early to pick up USE_*

PROG=	mount_9p
SRCS=	ninepuffs.c ninebuf.c nineproto.c fs.c node.c subr.c
LDADD+= -lpuffs -lutil

MAN=	mount_9p.8

.if (${USE_INET6} != "no")
CPPFLAGS += -DINET6
.endif

.include <bsd.prog.mk>
