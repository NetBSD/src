#	$NetBSD: Makefile,v 1.2.30.1 2023/01/23 13:43:07 martin Exp $

.include <bsd.own.mk>

PROG=	cxpm

CPPFLAGS+=	-I${X11SRCDIR.Xpm}/src

LDADD+=	-lXpm -lXt -lSM -lICE -lXext -lX11
DPADD+=	${LIBXPM} ${LIBXT} ${LIBSM} ${LIBICE} ${LIBXEXT} ${LIBX11}

.PATH:	${X11SRCDIR.Xpm}/${PROG}
.PATH:	${X11SRCDIR.Xpm}/man

.include "${NETBSDSRCDIR}/external/mit/xorg/lib/libXpm/xpm.mk"

.include <bsd.x11.mk>
.include <bsd.prog.mk>
