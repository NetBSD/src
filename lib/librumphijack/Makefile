#	$NetBSD: Makefile,v 1.7 2011/02/23 15:23:15 pooka Exp $
#

LIB=		rumphijack
NOSTATICLIB=	dlfun
NOPROFILE=	dlfun
LIBDPLIBS+=	pthread		${.CURDIR}/../libpthread 	\
		rumpclient	${.CURDIR}/../librumpclient
MAN=		rumphijack.3

SRCS=		hijack.c hijackdlsym.c

CPPFLAGS+=	-D_DIAGNOSTIC -D_REENTRANT

WARNS=		4

#DBG=-g
#NOGCCERROR=1
COPTS.hijackdlsym.c+=	-fno-optimize-sibling-calls

.include <bsd.lib.mk>
