#	$NetBSD: Makefile,v 1.7 2024/07/08 01:06:57 christos Exp $

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

COPTS.Eyes.c += -Wno-error=stack-protector

.PATH:	${X11SRCDIR.${PROG}}
.PATH:	${X11SRCDIR.${PROG}}/man

.include <bsd.x11.mk>
.include <bsd.prog.mk>
