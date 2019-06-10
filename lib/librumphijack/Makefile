#	$NetBSD: Makefile,v 1.20.10.1 2019/06/10 22:05:27 christos Exp $
#

LIB=		rumphijack
NOSTATICLIB=	dlfun
NOPROFILE=	dlfun
LIBDPLIBS+=	pthread		${.CURDIR}/../libpthread 	\
		rumpclient	${.CURDIR}/../librumpclient
MAN=		rumphijack.3

SRCS=		hijack.c hijackdlsym.c

CPPFLAGS+=	-D_DIAGNOSTIC -D_REENTRANT
CPPFLAGS.hijack.c+= -D_INCOMPLETE_XOPEN_C063

WARNS?=		5

# make sure the compiler doesn't get clever, since we need
# a stack frame
COPTS.hijackdlsym.c+=	-O0

.include <bsd.lib.mk>

CPPFLAGS+=	-U_FORTIFY_SOURCE
