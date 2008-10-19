#	$NetBSD: Makefile,v 1.1.1.1.6.2 2008/10/19 22:41:02 haad Exp $

.include <bsd.own.mk>

PROG=	xload
SRCS=	xload.c get_load.c get_rload.c

CPPFLAGS+=${X11FLAGS.VERSION}

APPDEFS=XLoad.ad

LDADD+=	-lXaw -lXmu -lXt -lSM -lICE -lXpm -lXext -lX11
DPADD+=	${LIBXAW} ${LIBXMU} ${LIBXT} ${LIBSM} ${LIBICE} \
	${LIBXPM} ${LIBXEXT} ${LIBX11}

.PATH:	${X11SRCDIR.${PROG}}

.include <bsd.x11.mk>
.include <bsd.prog.mk>
