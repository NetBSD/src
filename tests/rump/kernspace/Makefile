#	$NetBSD: Makefile,v 1.5.38.1 2018/01/16 13:01:10 martin Exp $
#

.include <bsd.own.mk>

LIB=	kernspace
SRCS=	thread.c busypage.c tsleep.c alloc.c lockme.c workqueue.c sendsig.c

RUMPTOP=${NETBSDSRCDIR}/sys/rump

LIBISPRIVATE=

.include "${RUMPTOP}/Makefile.rump"
.include <bsd.lib.mk>
.include <bsd.klinks.mk>
