#	$NetBSD: Makefile,v 1.1 2003/09/14 06:42:25 lukem Exp $

.include <bsd.own.mk>

PROG=		xsystrace
SRCS=		callbacks.c interface.c main.c

APPDEFS=	Xsystrace.ad

.PATH:		${X11SRCDIR.local}/programs/${PROG}/pixmaps
FILESDIR=	${X11INCDIR}/X11/pixmaps
FILES=		deny-always.xpm deny.xpm logo.xpm \
		permit-always.xpm permit.xpm skull.xpm

LDADD+=		-lXaw -lXmu -lXt -lSM -lICE -lXpm -lXext -lX11
DPADD+=		${LIBXAW} ${LIBXMU} ${LIBXT} ${LIBSM} ${LIBICE} \
		${LIBXPM} ${LIBXEXT} ${LIBX11}

.PATH:		${X11SRCDIR.local}/programs/${PROG}

.include <bsd.x11.mk>
.include <bsd.prog.mk>
