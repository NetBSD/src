#	$NetBSD: Makefile,v 1.2 2003/09/13 20:18:08 lukem Exp $

.include <bsd.own.mk>

PROG=	proxymngr
SRCS=	main.c pmdb.c config.c

CPPFLAGS.main.c=-DCONFIG_FILE=\"${X11LIBDIR}/proxymngr/pmconfig\"

CPPSCRIPTS=	pmconfig
CPPSCRIPTFLAGS=	-DLBXPROXY=${X11BINDIR}/lbxproxy
FILES=		pmconfig
FILESDIR=	${X11LIBDIR}/proxymngr

INCS=	PM.h PMproto.h
INCSDIR=${X11INCDIR}/X11/PM

LDADD+=	-lXt -lSM -lICE -lXext -lX11
DPADD+=	${LIBXT} ${LIBSM} ${LIBICE} ${LIBXEXT} ${LIBX11}

.PATH:	${X11SRCDIR.xc}/programs/${PROG}

.include <bsd.x11.mk>
.include <bsd.prog.mk>
