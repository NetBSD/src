#	$NetBSD: Makefile,v 1.3 2003/09/20 06:20:45 lukem Exp $

.include <bsd.own.mk>

PROG=	beforelight
SRCS=	b4light

APPDEFS=Beforelight.ad

BUILDSYMLINKS=	b4light.man beforelight.man \
		B4light.ad Beforelight.ad

LDADD+=	-lXss -lXaw -lXmu -lXt -lSM -lICE -lXpm -lXext -lX11
DPADD+=	${LIBXSS} ${LIBXAW} ${LIBXMU} ${LIBXT} ${LIBSM} ${LIBICE} \
	${LIBXPM} ${LIBXEXT} ${LIBX11}

.PATH:	${X11SRCDIR.xc}/programs/${PROG}

.include <bsd.x11.mk>
.include <bsd.prog.mk>
