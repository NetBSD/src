#	$NetBSD: Makefile,v 1.2.12.1 2013/06/23 06:28:41 tls Exp $

.include <bsd.own.mk>

PROG=	xmessage
SRCS=	xmessage.c makeform.c readfile.c

APPDEFS=Xmessage Xmessage-color

LDADD+=	-lXaw -lXmu -lXt -lSM -lICE -lXpm -lXext -lX11
DPADD+=	${LIBXAW} ${LIBXMU} ${LIBXT} ${LIBSM} ${LIBICE} \
	${LIBXPM} ${LIBXEXT} ${LIBX11}

.PATH:	${X11SRCDIR.${PROG}}
.PATH:	${X11SRCDIR.${PROG}}/app-defaults

.include <bsd.x11.mk>
.include <bsd.prog.mk>
