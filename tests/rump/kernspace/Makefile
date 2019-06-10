#	$NetBSD: Makefile,v 1.6.4.1 2019/06/10 22:10:10 christos Exp $
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
