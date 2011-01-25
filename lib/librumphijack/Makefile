#	$NetBSD: Makefile,v 1.4 2011/01/25 12:56:25 pooka Exp $
#

LIB=		rumphijack
NOSTATICLIB=	dlfun
NOPROFILE=	dlfun
LIBDPLIBS+=	pthread ${.CURDIR}/../libpthread
MAN=		rumphijack.3

SRCS=		hijack.c

CPPFLAGS+=	-D_DIAGNOSTIC -D_REENTRANT

#DBG=-g
#NOGCCERROR=1

.include <bsd.lib.mk>
