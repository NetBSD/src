#	$NetBSD: Makefile,v 1.4 2010/11/30 14:08:39 pooka Exp $
#

WARNS=		4

# rumpuser.h is in sys/rump for inclusion by kernel components
.PATH:		${.CURDIR}/../../sys/rump/include/rump

LIB=		rumpuser
LIBDPLIBS+=	pthread ${.CURDIR}/../libpthread
#CPPFLAGS+=	-D_DIAGNOSTIC

SRCS=		rumpuser.c rumpuser_net.c
SRCS+=		rumpuser_pth.c
SRCS+=		rumpuser_dl.c rumpuser_sp.c

INCSDIR=	/usr/include/rump
INCS=		rumpuser.h

MAN=		rumpuser.3

CPPFLAGS+=	-D_REENTRANT


.include <bsd.lib.mk>
