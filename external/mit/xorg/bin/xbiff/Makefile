#	$NetBSD: Makefile,v 1.3.12.1 2025/08/02 05:50:38 perseant Exp $

.include <bsd.own.mk>

PROG=	xbiff
SRCS=	xbiff.c Mailbox.c

LDADD+=	-lXext -lXaw -lXmu -lXt -lX11 -lSM -lICE -lXau
DPADD+=	${LIBXEXT} ${LIBXAW} ${LIBXMU} ${LIBXT} ${LIBX11} ${LIBSM} ${LIBICE} \
	${LIBXAU}

CPPFLAGS+=	-I${X11SRCDIR.${PROG}}/../include
CPPFLAGS+=	-D_CONST_X_STRING
CPPFLAGS+=	-DPACKAGE_STRING=\"${PACKAGE_STRING}\"

.PATH:	${X11SRCDIR.${PROG}}
.PATH:	${X11SRCDIR.${PROG}}/man

.include <bsd.x11.mk>
.include <bsd.prog.mk>
