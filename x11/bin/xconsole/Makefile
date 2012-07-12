#	$NetBSD: Makefile,v 1.4 2012/07/12 10:14:04 wiz Exp $

.include <bsd.own.mk>

PROG=	xconsole

APPDEFS=XConsole.ad

CPPFLAGS+=-DHAS_OPENPTY
LDADD+=	-lutil -lXaw -lXmu -lXt -lSM -lICE -lXpm -lXext -lX11
DPADD+=	${LIBUTIL} ${LIBXAW} ${LIBXMU} ${LIBXT} ${LIBSM} ${LIBICE} \
	${LIBXPM} ${LIBXEXT} ${LIBX11}

.PATH:	${X11SRCDIR.xc}/programs/${PROG}

.include <bsd.x11.mk>
.include <bsd.prog.mk>
