#	$NetBSD: Makefile,v 1.2 2003/09/12 08:08:14 lukem Exp $

.include <bsd.own.mk>

LIB=	Xmuu
.PATH:	${X11SRCDIR.xc}/lib/${LIB}
.PATH:	${X11SRCDIR.xc}/lib/Xmu

SRCS=	ClientWin.c CursorName.c DefErrMsg.c GetHost.c Lower.c

CPPFLAGS+=-DHAS_SNPRINTF

LDADD+=	-lXt -lSM -lICE -lXext -lX11
DPADD+=	${LIBXT} ${LIBSM} ${LIBICE} ${LIBXEXT} ${LIBX11}

.include <bsd.lib.mk>
.include <bsd.x11.mk>
