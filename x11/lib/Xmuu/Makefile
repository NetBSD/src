#	$NetBSD: Makefile,v 1.3 2003/09/13 20:18:49 lukem Exp $

.include <bsd.own.mk>

LIB=	Xmuu
.PATH:	${X11SRCDIR.xc}/lib/${LIB}
.PATH:	${X11SRCDIR.xc}/lib/Xmu

SRCS=	ClientWin.c CursorName.c DefErrMsg.c GetHost.c Lower.c

CPPFLAGS+=-DHAS_SNPRINTF

LDADD+=	-lXt -lSM -lICE -lXext -lX11
DPADD+=	${LIBXT} ${LIBSM} ${LIBICE} ${LIBXEXT} ${LIBX11}

.include <bsd.x11.mk>
.include <bsd.lib.mk>
