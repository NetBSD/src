#	$NetBSD: Makefile,v 1.19 2014/07/09 23:41:40 justin Exp $
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
SRCS+=		rumpuser_component.c rumpuser_bio.c
SRCS+=		rumpuser_file.c rumpuser_mem.c

SRCS+=		rumpuser_errtrans.c rumpuser_sigtrans.c

# optional
SRCS+=		rumpuser_dl.c rumpuser_sp.c rumpuser_daemonize.c

INCSDIR=	/usr/include/rump
INCS=		rumpuser.h rumpuser_component.h rumpuser_port.h

MAN=		rumpuser.3

CPPFLAGS+=	-D_REENTRANT


.include <bsd.lib.mk>
