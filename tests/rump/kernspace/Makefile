#	$NetBSD: Makefile,v 1.6 2017/09/29 12:42:36 maya Exp $
#

.include <bsd.own.mk>

LIB=	kernspace
SRCS=	thread.c busypage.c tsleep.c alloc.c lockme.c workqueue.c sendsig.c

RUMPTOP=${NETBSDSRCDIR}/sys/rump

LIBISPRIVATE=

.include "${RUMPTOP}/Makefile.rump"
.include <bsd.lib.mk>
.include <bsd.klinks.mk>
