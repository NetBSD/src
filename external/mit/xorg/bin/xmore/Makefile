#	$NetBSD: Makefile,v 1.3 2010/08/14 09:25:35 mrg Exp $

.include <bsd.own.mk>

PROG=		xmore
APPDEFS=	XMore

SRCS=	xmore.c

CPPFLAGS+=	-DXMORE

LDADD+=	-lXaw -lXmu -lXt -lSM -lICE -lXpm -lXext -lX11
DPADD+=	${LIBXAW} ${LIBXMU} ${LIBXT} ${LIBSM} \
	${LIBICE} ${LIBXPM} ${LIBXEXT} ${LIBX11}

.PATH:	${X11SRCDIR.${PROG}}

.include <bsd.x11.mk>
.include <bsd.prog.mk>
