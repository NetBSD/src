#	$NetBSD: Makefile,v 1.3 2004/11/10 03:21:02 christos Exp $

.include <bsd.own.mk>

PROG=	xconsole

APPDEFS=XConsole.ad

LDADD+=	-lutil -lXaw -lXmu -lXt -lSM -lICE -lXpm -lXext -lX11
DPADD+=	${LIBUTIL} ${LIBXAW} ${LIBXMU} ${LIBXT} ${LIBSM} ${LIBICE} \
	${LIBXPM} ${LIBXEXT} ${LIBX11}

.PATH:	${X11SRCDIR.xc}/programs/${PROG}

.include <bsd.x11.mk>
.include <bsd.prog.mk>
