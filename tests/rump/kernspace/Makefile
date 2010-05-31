#	$NetBSD: Makefile,v 1.1 2010/05/31 23:32:51 pooka Exp $
#

LIB=	kernspace
SRCS=	thread.c busypage.c tsleep.c

RUMPTOP=${NETBSDSRCDIR}/sys/rump

LIBISPRIVATE=

.include "${RUMPTOP}/Makefile.rump"
.include <bsd.lib.mk>
.include <bsd.klinks.mk>
