#	$NetBSD: Makefile,v 1.6 2011/01/27 18:12:19 pooka Exp $
#

LIB=		rumphijack
NOSTATICLIB=	dlfun
NOPROFILE=	dlfun
LIBDPLIBS+=	pthread		${.CURDIR}/../libpthread 	\
		rumpclient	${.CURDIR}/../librumpclient
MAN=		rumphijack.3

SRCS=		hijack.c

CPPFLAGS+=	-D_DIAGNOSTIC -D_REENTRANT

WARNS=		4

#DBG=-g
#NOGCCERROR=1

.include <bsd.lib.mk>
