#	$NetBSD: Makefile,v 1.9 2013/03/18 13:14:10 pooka Exp $
#

WARNS?=		5

# rumpuser.h is in sys/rump for inclusion by kernel components
.PATH:		${.CURDIR}/../../sys/rump/include/rump

LIB=		rumpuser
LIBDPLIBS+=	pthread ${.CURDIR}/../libpthread
CPPFLAGS+=	-DLIBRUMPUSER
#CPPFLAGS+=	-D_DIAGNOSTIC

SRCS=		rumpuser.c
SRCS+=		rumpuser_pth.c
SRCS+=		rumpuser_dl.c rumpuser_sp.c rumpuser_daemonize.c
SRCS+=		rumpuser_component.c

INCSDIR=	/usr/include/rump
INCS=		rumpuser.h rumpuser_component.h

MAN=		rumpuser.3

CPPFLAGS+=	-D_REENTRANT


.include <bsd.lib.mk>
