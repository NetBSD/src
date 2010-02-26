#	$NetBSD: Makefile,v 1.2 2010/02/26 18:54:20 pooka Exp $
#

WARNS=		4

# rumpuser.h is in sys/rump for inclusion by kernel components
.PATH:		${.CURDIR}/../../sys/rump/include/rump

LIB=		rumpuser
LIBDPLIBS+=     pthread ${.CURDIR}/../libpthread

SRCS=		rumpuser.c rumpuser_net.c
SRCS+=		rumpuser_pth.c
SRCS+=		rumpuser_dl.c

INCSDIR=	/usr/include/rump
INCS=		rumpuser.h

MAN=		rumpuser.3

CPPFLAGS+=	-D_REENTRANT


.include <bsd.lib.mk>
