#	$NetBSD: Makefile,v 1.3 2004/05/16 10:25:14 lukem Exp $

.include <bsd.own.mk>

PROG=	proxymngr
SRCS=	main.c pmdb.c config.c

PROXYMANAGERDIR=	${X11ETCDIR}/proxymngr

CPPFLAGS.main.c=	-DCONFIG_FILE=\"${PROXYMANAGERDIR}/pmconfig\"

X11EXTRAMANDEFS+=	-DPROXYMANAGERDIR=${PROXYMANAGERDIR}

CPPSCRIPTS=	pmconfig
CPPSCRIPTFLAGS=	-DLBXPROXY=${X11BINDIR}/lbxproxy
CONFIGFILES=	pmconfig
FILESDIR=	${PROXYMANAGERDIR}

INCS=	PM.h PMproto.h
INCSDIR=${X11INCDIR}/X11/PM

LDADD+=	-lXt -lSM -lICE -lXext -lX11
DPADD+=	${LIBXT} ${LIBSM} ${LIBICE} ${LIBXEXT} ${LIBX11}

.PATH:	${X11SRCDIR.xc}/programs/${PROG}

.include <bsd.x11.mk>
.include <bsd.prog.mk>
