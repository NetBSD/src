#	$NetBSD: Makefile,v 1.12 2013/07/29 10:13:21 pooka Exp $
#

LIB=		rumphijack
NOSTATICLIB=	dlfun
NOPROFILE=	dlfun
LIBDPLIBS+=	pthread		${.CURDIR}/../libpthread 	\
		rumpclient	${.CURDIR}/../librumpclient
MAN=		rumphijack.3

SRCS=		hijack.c hijackdlsym.c

CPPFLAGS+=	-D_DIAGNOSTIC -D_REENTRANT -I${.CURDIR}/../librumpuser

WARNS?=		5

# make sure the compiler doesn't get clever, since we need
# a stack frame
COPTS.hijackdlsym.c+=	-O0

.include <bsd.sys.mk>

CPPFLAGS+=	-U_FORTIFY_SOURCE

.include <bsd.lib.mk>
