#	$NetBSD: Makefile,v 1.1.1.1.20.1 2011/03/05 15:09:09 bouyer Exp $

.include <bsd.own.mk>

PROG=	xbiff
SRCS=	xbiff.c Mailbox.c

LDADD+=	-lXext -lXaw -lXmu -lXt -lX11 -lSM -lICE -lXau
DPADD+=	${LIBXEXT} ${LIBXAW} ${LIBXMU} ${LIBXT} ${LIBX11} ${LIBSM} ${LIBICE} \
	${LIBXAU}

CPPFLAGS+=	-I${X11SRCDIR.${PROG}}/../include

.PATH:	${X11SRCDIR.${PROG}}
.PATH:	${X11SRCDIR.${PROG}}/man

.include <bsd.x11.mk>
.include <bsd.prog.mk>
