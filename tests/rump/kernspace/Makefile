#	$NetBSD: Makefile,v 1.4 2011/01/06 13:12:52 pooka Exp $
#

.include <bsd.own.mk>

LIB=	kernspace
SRCS=	thread.c busypage.c tsleep.c alloc.c lockme.c

RUMPTOP=${NETBSDSRCDIR}/sys/rump

LIBISPRIVATE=

.include "${RUMPTOP}/Makefile.rump"
.include <bsd.lib.mk>
.include <bsd.klinks.mk>
