#	$NetBSD: Makefile,v 1.1.1.1.2.2 2008/09/18 05:18:28 wrstuden Exp $

.include <bsd.own.mk>

PROG=	xconsole

APPDEFS=XConsole.ad

LDADD+=	-lutil -lXaw -lXmu -lXt -lSM -lICE -lXpm -lXext -lX11
DPADD+=	${LIBUTIL} ${LIBXAW} ${LIBXMU} ${LIBXT} ${LIBSM} ${LIBICE} \
	${LIBXPM} ${LIBXEXT} ${LIBX11}

.PATH:	${X11SRCDIR.${PROG}}

.include <bsd.x11.mk>
.include <bsd.prog.mk>
