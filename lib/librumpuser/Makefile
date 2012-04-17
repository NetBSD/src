#	$NetBSD: Makefile,v 1.5.6.1 2012/04/17 00:05:33 yamt Exp $
#

WARNS?=		5

# rumpuser.h is in sys/rump for inclusion by kernel components
.PATH:		${.CURDIR}/../../sys/rump/include/rump

LIB=		rumpuser
LIBDPLIBS+=	pthread ${.CURDIR}/../libpthread
#CPPFLAGS+=	-D_DIAGNOSTIC

SRCS=		rumpuser.c rumpuser_net.c
SRCS+=		rumpuser_pth.c
SRCS+=		rumpuser_dl.c rumpuser_sp.c rumpuser_daemonize.c

INCSDIR=	/usr/include/rump
INCS=		rumpuser.h

MAN=		rumpuser.3

CPPFLAGS+=	-D_REENTRANT


.include <bsd.lib.mk>
