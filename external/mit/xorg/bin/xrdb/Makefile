#	$NetBSD: Makefile,v 1.7 2016/04/13 22:01:55 christos Exp $

.include <bsd.own.mk>

PROG=	xrdb

CPPFLAGS+=-DCPP="\"/usr/bin/cpp -traditional\"" -DHAVE_MKSTEMP	# XXX
CPPFLAGS+=-DHAVE_ASPRINTF -DPACKAGE_STRING="\"xrdb 1.1.0\""

LDADD+=	-lXmuu -lXt -lSM -lICE -lXext -lX11
DPADD+=	${LIBXMUU} ${LIBXT} ${LIBSM} ${LIBICE} ${LIBXEXT} ${LIBX11}

.PATH:	${X11SRCDIR.${PROG}}
.PATH:	${X11SRCDIR.${PROG}}/man

.include <bsd.x11.mk>
.include <bsd.prog.mk>
