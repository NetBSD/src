#	$NetBSD: Makefile,v 1.14 2014/01/10 15:00:16 martin Exp $
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

.include <bsd.own.mk>	# prerequisite for bsd.sys.mk
.include <bsd.sys.mk>

CPPFLAGS+=	-U_FORTIFY_SOURCE

.include <bsd.lib.mk>
