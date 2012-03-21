#	$NetBSD: Makefile,v 1.9 2012/03/21 05:37:44 matt Exp $
#

LIB=		rumphijack
NOSTATICLIB=	dlfun
NOPROFILE=	dlfun
LIBDPLIBS+=	pthread		${.CURDIR}/../libpthread 	\
		rumpclient	${.CURDIR}/../librumpclient
MAN=		rumphijack.3

SRCS=		hijack.c hijackdlsym.c

CPPFLAGS+=	-D_DIAGNOSTIC -D_REENTRANT

WARNS?=		5

# make sure the compiler doesn't get clever, since we need
# a stack frame
COPTS.hijackdlsym.c+=	-O0

.include <bsd.lib.mk>
