#	$NetBSD: Makefile,v 1.3 2023/01/19 05:31:34 mrg Exp $

.include <bsd.own.mk>

PROG=	sxpm

CPPFLAGS+=	-I${X11SRCDIR.Xpm}/src
CPPFLAGS+=	-I${DESTDIR}${X11INCDIR}/X11

LDADD+=	-lXpm -lXt -lSM -lICE -lXext -lX11
DPADD+=	${LIBXPM} ${LIBXT} ${LIBSM} ${LIBICE} ${LIBXEXT} ${LIBX11}

.PATH:	${X11SRCDIR.Xpm}/${PROG}
.PATH:	${X11SRCDIR.Xpm}/man

.include "${NETBSDSRCDIR}/external/mit/xorg/lib/libXpm/xpm.mk"

.include <bsd.x11.mk>
.include <bsd.prog.mk>
