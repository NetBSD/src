#	$NetBSD: Makefile,v 1.5 2022/12/25 23:05:58 christos Exp $

.include <bsd.own.mk>

CPPFLAGS+=-DXRENDER -DPRESENT

PROG=	xeyes
SRCS=	xeyes.c Eyes.c transform.c

LDADD+=	-lXi -lXmu -lXt -lSM -lICE -lXext -lXrender -lX11 -lm \
	-lxcb-xfixes -lxcb-damage -lxcb-present -lX11-xcb -lxcb
DPADD+=	${LIBXMU} ${LIBXT} ${LIBSM} ${LIBICE} ${LIBXEXT} ${LIBX11} ${LIBM} \
	${LIBXCB_XFIXES} ${LIBXCB_DAMAGE} ${LIBXCB_PRESENT} ${LIBX11_XCB} \
	${LIBXCB}

.PATH:	${X11SRCDIR.${PROG}}
.PATH:	${X11SRCDIR.${PROG}}/man

.include <bsd.x11.mk>
.include <bsd.prog.mk>
