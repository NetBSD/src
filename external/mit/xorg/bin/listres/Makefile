#	$NetBSD: Makefile,v 1.2.40.1 2025/08/02 05:50:38 perseant Exp $

.include <bsd.own.mk>

PROG=	listres

LDADD+=	-lXaw -lXmu -lXt -lSM -lICE -lXpm -lXext -lX11
DPADD+=	${LIBXAW} ${LIBXMU} ${LIBXT} ${LIBSM} ${LIBICE} \
	${LIBXPM} ${LIBXEXT} ${LIBX11}

CPPFLAGS+=	-DPACKAGE_STRING=\"${PACKAGE_STRING}\"

.PATH:	${X11SRCDIR.${PROG}}
.PATH:	${X11SRCDIR.${PROG}}/man

.include <bsd.x11.mk>
.include <bsd.prog.mk>
