#	$NetBSD: Makefile,v 1.6 2023/01/08 12:00:21 mrg Exp $

.include <bsd.own.mk>

PROG=	xhost

CPPFLAGS+=${X11FLAGS.CONNECTION} -DBSD44SOCKETS
CPPFLAGS+=-DHAVE_INET_ATON
CPPFLAGS+=-DUSE_GETTEXT -DLOCALEDIR=\"${LOCALEDIR}\"

.if (${USE_INET6} != "no")
CPPFLAGS+=      -DIPv6
.endif

LDADD+=	-lXmuu -lXt -lSM -lICE -lXext -lX11 -lintl
DPADD+=	${LIBXMUU} ${LIBXT} ${LIBSM} ${LIBICE} ${LIBXEXT} ${LIBX11} ${LIBINTL}

.PATH:	${X11SRCDIR.${PROG}}
.PATH:	${X11SRCDIR.${PROG}}/man

.include <bsd.x11.mk>
.include <bsd.prog.mk>
