#	$NetBSD: Makefile,v 1.4 2004/03/17 23:19:48 rtr Exp $

.include <bsd.own.mk>

PROG=	xf86config

.PATH:	${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/xf86config

SRCS=	xf86config.c cards.c

LDADD=	-lxkbfile -lXext -lX11
DPADD=	${LIBXKBFILE} ${LIBXEXT} ${LIBX11}

CPPFLAGS+=	-DCARD_DATABASE_FILE='"${X11USRLIBDIR}/X11/Cards"' \
		-DXKB_RULES_DIR=\"${X11USRLIBDIR}/X11/xkb/rules\" \
		-DXCONFIGFILE=\"XF86Config\" \
		-DXVERSION="${XVERSION}" \
		-DWSCONS_SUPPORT

.if ${MACHINE} == "i386"
CPPFLAGS+=	-DPCCONS_SUPPORT -DPCVT_SUPPORT
.endif

.include <bsd.x11.mk>
XVERSION!=	${PRINTX11VERSION}
.include <bsd.prog.mk>
