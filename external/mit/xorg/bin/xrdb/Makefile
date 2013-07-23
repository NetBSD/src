#	$NetBSD: Makefile,v 1.3.14.1 2013/07/23 21:07:28 riastradh Exp $

.include <bsd.own.mk>

PROG=	xrdb

CPPFLAGS+=-DCPP="\"/usr/bin/cpp -traditional\"" -DHAS_MKSTEMP	# XXX
CPPFLAGS+=-DHAVE_ASPRINTF

LDADD+=	-lXmuu -lXt -lSM -lICE -lXext -lX11
DPADD+=	${LIBXMUU} ${LIBXT} ${LIBSM} ${LIBICE} ${LIBXEXT} ${LIBX11}

.PATH:	${X11SRCDIR.${PROG}}
.PATH:	${X11SRCDIR.${PROG}}/man

.include <bsd.x11.mk>
.include <bsd.prog.mk>
