#	$NetBSD: Makefile,v 1.14 2013/05/15 20:47:20 pooka Exp $
#

.include <bsd.own.mk>

WARNS?=		5

# rumpuser.h is in sys/rump for inclusion by kernel components
.PATH:		${.CURDIR}/../../sys/rump/include/rump

LIB=		rumpuser
LIBDPLIBS+=	pthread ${.CURDIR}/../libpthread
.for lib in ${RUMPUSER_EXTERNAL_DPLIBS}
LIBDO.${lib}=	_external
LIBDPLIBS+=	${lib} lib
.endfor
CPPFLAGS+=	-DLIBRUMPUSER
#CPPFLAGS+=	-D_DIAGNOSTIC

SRCS=		rumpuser.c
SRCS+=		rumpuser_pth.c
SRCS+=		rumpuser_component.c rumpuser_errtrans.c rumpuser_bio.c

# optional
SRCS+=		rumpuser_dl.c rumpuser_sp.c rumpuser_daemonize.c

INCSDIR=	/usr/include/rump
INCS=		rumpuser.h rumpuser_component.h

MAN=		rumpuser.3

CPPFLAGS+=	-D_REENTRANT


.include <bsd.lib.mk>
