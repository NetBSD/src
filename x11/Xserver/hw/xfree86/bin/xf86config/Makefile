#	$NetBSD: Makefile,v 1.4.4.1 2004/11/15 09:12:39 rtr Exp $

.include <bsd.own.mk>

PROG=	xorgconfig

.PATH:	${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/xf86config

SRCS=	xorgconfig.c cards.c

LDADD=	-lxkbfile -lXext -lX11
DPADD=	${LIBXKBFILE} ${LIBXEXT} ${LIBX11}

CPPFLAGS+=	-DCARD_DATABASE_FILE='"${X11USRLIBDIR}/X11/Cards"' \
		-DXKB_RULES_DIR=\"${X11USRLIBDIR}/X11/xkb/rules\" \
		-DXCONFIGFILE=\"XF86Config\" \
		-DXVERSION="${XVERSION}" \
		-DWSCONS_SUPPORT -D__XKBDEFRULES__='"xorg"' \
		-D__XSERVERNAME__='"Xorg"' \
		-DXVENDORNAME='"The X.Org Foundation"' \
		-DFILEMANSUFFIX=\"5\" \
		-DXVERSIONSTRING=\"6.8.1\"

.if ${MACHINE} == "i386"
CPPFLAGS+=	-DPCCONS_SUPPORT -DPCVT_SUPPORT
.endif

.include <bsd.x11.mk>
XVERSION!=	${PRINTX11VERSION}
.include <bsd.prog.mk>
