#	$NetBSD: Makefile,v 1.3 2010/06/14 21:06:09 pooka Exp $
#

.include <bsd.own.mk>

LIB=	kernspace
SRCS=	thread.c busypage.c tsleep.c alloc.c

RUMPTOP=${NETBSDSRCDIR}/sys/rump

LIBISPRIVATE=

.include "${RUMPTOP}/Makefile.rump"
.include <bsd.lib.mk>
.include <bsd.klinks.mk>
