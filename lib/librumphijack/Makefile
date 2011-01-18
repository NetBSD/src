#	$NetBSD: Makefile,v 1.3 2011/01/18 19:53:23 pooka Exp $
#

LIB=		rumphijack
NOSTATICLIB=	dlfun
NOPROFILE=	dlfun
LIBDPLIBS+=	pthread ${.CURDIR}/../libpthread

SRCS=		hijack.c

CPPFLAGS+=	-D_DIAGNOSTIC -D_REENTRANT

#DBG=-g
#NOGCCERROR=1

.include <bsd.lib.mk>
