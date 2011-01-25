#	$NetBSD: Makefile,v 1.5 2011/01/25 12:56:57 pooka Exp $
#

LIB=		rumphijack
NOSTATICLIB=	dlfun
NOPROFILE=	dlfun
LIBDPLIBS+=	pthread ${.CURDIR}/../libpthread
MAN=		rumphijack.3

SRCS=		hijack.c

CPPFLAGS+=	-D_DIAGNOSTIC -D_REENTRANT

WARNS=		4

#DBG=-g
#NOGCCERROR=1

.include <bsd.lib.mk>
