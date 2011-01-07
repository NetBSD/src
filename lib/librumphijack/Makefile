#	$NetBSD: Makefile,v 1.1 2011/01/07 19:52:43 pooka Exp $
#

LIB=		rumphijack
NOSTATICLIB=	dlfun
NOPROFILE=	dlfun
LIBDPLIBS+=	pthread /usr/lib

SRCS=		hijack.c

CPPFLAGS+=	-D_DIAGNOSTIC

DBG=-g
NOGCCERROR=1

.include <bsd.lib.mk>
