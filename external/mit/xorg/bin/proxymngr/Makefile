#	$NetBSD: Makefile,v 1.2.62.1 2019/06/10 22:02:35 christos Exp $

.include <bsd.own.mk>

PROG=	proxymngr
SRCS=	main.c pmdb.c config.c

PROXYMANAGERDIR=	${X11ETCDIR}/proxymngr

CPPFLAGS.main.c=	-DCONFIG_FILE=\"${PROXYMANAGERDIR}/pmconfig\" \
			-DXVENDORNAMESHORT=${XVENDORNAMESHORT} \
                	-DXVENDORNAME=${XVENDORNAME} \
			-DXORG_RELEASE=${XORG_RELEASE}


X11EXTRAMANDEFS+=	-e 's,PROXYMANAGERDIR,${PROXYMANAGERDIR},g'

CPPSCRIPTS=	pmconfig
CPPSCRIPTFLAGS=	-DLBXPROXY=${X11BINDIR}/lbxproxy
CONFIGFILES=	pmconfig
FILESDIR=	${PROXYMANAGERDIR}

LDADD+=	-lXt -lSM -lICE -lXext -lX11
DPADD+=	${LIBXT} ${LIBSM} ${LIBICE} ${LIBXEXT} ${LIBX11}

.if (${USE_INET6} != "no")
CPPFLAGS+=      -DIPv6
.endif

X11EXTRAMANTRANSFORMS+= \
	__configdir__		${X11ETCDIR}/proxymngr

.PATH:	${X11SRCDIR.${PROG}}

.include <bsd.x11.mk>
.include <bsd.prog.mk>
