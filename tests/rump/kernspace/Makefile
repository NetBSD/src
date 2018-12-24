#	$NetBSD: Makefile,v 1.7 2018/12/24 21:42:05 thorpej Exp $
#

.include <bsd.own.mk>

LIB=	kernspace
SRCS=	thread.c threadpool.c busypage.c tsleep.c alloc.c lockme.c \
	workqueue.c sendsig.c

RUMPTOP=${NETBSDSRCDIR}/sys/rump

LIBISPRIVATE=

.include "${RUMPTOP}/Makefile.rump"
.include <bsd.lib.mk>
.include <bsd.klinks.mk>
