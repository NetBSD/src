#	$NetBSD: Makefile,v 1.15 2014/01/10 15:54:29 christos Exp $
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

CPPFLAGS+=	-U_FORTIFY_SOURCE

.include <bsd.lib.mk>
