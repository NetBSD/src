#	$NetBSD: Makefile,v 1.1 2013/06/16 01:51:29 christos Exp $

NOMAN=	1

.include <bsd.own.mk>

SHLIB_MAJOR=0
SHLIB_MINOR=0
LIB=	XdmGreet
SRCS=	Login.c greet.c verify.c
CPPFLAGS+= -DRETSIGTYPE=void

LDADD+=	-lXaw -lXmu -lXt -lSM -lICE -lXpm -lXext -lX11 -lXdmcp -lXinerama
DPADD+=	${LIBXAW} ${LIBXMU} ${LIBXT} ${LIBSM} ${LIBICE} \
	${LIBXPM} ${LIBXEXT} ${LIBX11} ${LIBXDMCP} ${LIBXINERAMA}

.include "../Makefile.xdm"

.include <bsd.x11.mk>
.include <bsd.lib.mk>

.PATH: ${X11SRCDIR.xdm}/greeter
