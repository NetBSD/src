#	$NetBSD: Makefile,v 1.2 2011/01/18 15:00:22 pooka Exp $
#

LIB=		rumphijack
NOSTATICLIB=	dlfun
NOPROFILE=	dlfun
LIBDPLIBS+=	pthread /usr/lib

SRCS=		hijack.c

CPPFLAGS+=	-D_DIAGNOSTIC -D_REENTRANT

DBG=-g
NOGCCERROR=1

.include <bsd.lib.mk>
