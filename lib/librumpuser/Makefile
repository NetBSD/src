#	$NetBSD: Makefile,v 1.3 2010/10/27 20:44:50 pooka Exp $
#

WARNS=		4

# rumpuser.h is in sys/rump for inclusion by kernel components
.PATH:		${.CURDIR}/../../sys/rump/include/rump

LIB=		rumpuser
LIBDPLIBS+=     pthread ${.CURDIR}/../libpthread

SRCS=		rumpuser.c rumpuser_net.c
SRCS+=		rumpuser_pth.c
SRCS+=		rumpuser_dl.c rumpuser_sp.c

INCSDIR=	/usr/include/rump
INCS=		rumpuser.h

MAN=		rumpuser.3

CPPFLAGS+=	-D_REENTRANT


.include <bsd.lib.mk>
