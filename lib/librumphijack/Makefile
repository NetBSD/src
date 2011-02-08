#	$NetBSD: Makefile,v 1.3.2.1 2011/02/08 16:19:04 bouyer Exp $
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
