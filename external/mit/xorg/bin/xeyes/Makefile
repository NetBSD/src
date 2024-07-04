#	$NetBSD: Makefile,v 1.6 2024/07/04 00:33:28 mrg Exp $

.include <bsd.own.mk>

CPPFLAGS+=-DXRENDER -DPRESENT

PROG=	xeyes
SRCS=	xeyes.c Eyes.c transform.c

LDADD+=	-lXi -lXmu -lXt -lSM -lICE -lXext -lXrender -lX11 -lm \
	-lxcb-xfixes -lxcb-damage -lxcb-present -lX11-xcb -lxcb
DPADD+=	${LIBXMU} ${LIBXT} ${LIBSM} ${LIBICE} ${LIBXEXT} ${LIBX11} ${LIBM} \
	${LIBXCB_XFIXES} ${LIBXCB_DAMAGE} ${LIBXCB_PRESENT} ${LIBX11_XCB} \
	${LIBXCB}

CPPFLAGS+=	-DPACKAGE_STRING=\"${PACKAGE_STRING}\"

.PATH:	${X11SRCDIR.${PROG}}
.PATH:	${X11SRCDIR.${PROG}}/man

.include <bsd.x11.mk>
.include <bsd.prog.mk>
