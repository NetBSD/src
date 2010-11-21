#	$NetBSD: Makefile,v 1.2 2010/11/21 02:33:42 mrg Exp $

.include <bsd.own.mk>

PROG=	xmessage
SRCS=	xmessage.c makeform.c readfile.c

APPDEFS=Xmessage.ad Xmessage-color.ad

LDADD+=	-lXaw -lXmu -lXt -lSM -lICE -lXpm -lXext -lX11
DPADD+=	${LIBXAW} ${LIBXMU} ${LIBXT} ${LIBSM} ${LIBICE} \
	${LIBXPM} ${LIBXEXT} ${LIBX11}

.PATH:	${X11SRCDIR.${PROG}}

.include <bsd.x11.mk>
.include <bsd.prog.mk>
