#	$NetBSD: Makefile,v 1.4 2021/08/23 22:14:31 mrg Exp $

.include <bsd.own.mk>

CPPFLAGS+=-DXRENDER -DPRESENT

PROG=	xeyes
SRCS=	xeyes.c Eyes.c transform.c

LDADD+=	-lXi -lXmu -lXt -lSM -lICE -lXext -lXrender -lX11 -lm \
	-lxcb-xfixes -lxcb-damage -lxcb-present -lX11-xcb
DPADD+=	${LIBXMU} ${LIBXT} ${LIBSM} ${LIBICE} ${LIBXEXT} ${LIBX11} ${LIBM} \
	${LIBXCB_XFIXES} ${LIBXCB_DAMAGE} ${LIBXCB_PRESENT} ${LIBX11_XCB} 

.PATH:	${X11SRCDIR.${PROG}}
.PATH:	${X11SRCDIR.${PROG}}/man

.include <bsd.x11.mk>
.include <bsd.prog.mk>
