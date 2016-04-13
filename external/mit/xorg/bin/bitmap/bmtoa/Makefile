#	$NetBSD: Makefile,v 1.2 2016/04/13 21:48:21 christos Exp $

NOMAN=	1

.include <bsd.own.mk>

PROG=	bmtoa

CPPFLAGS+=-DHAVE_MKSTEMP
LDADD+=	-lXaw -lXmu -lXt -lSM -lICE -lXpm -lXext -lX11 -lm
DPADD+=	${LIBXAW} ${LIBXMU} ${LIBXT} ${LIBSM} ${LIBICE} \
	${LIBXPM} ${LIBXEXT} ${LIBX11} ${LIBM}

.PATH:	${X11SRCDIR.bitmap}

.include <bsd.x11.mk>
.include <bsd.prog.mk>
