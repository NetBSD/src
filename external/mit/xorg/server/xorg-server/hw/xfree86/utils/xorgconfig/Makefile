#	$NetBSD: Makefile,v 1.2 2008/10/14 23:37:19 cube Exp $

.include <bsd.own.mk>

PROG=	xorgconfig

.PATH:	${X11SRCDIR.xorg-server}/hw/xfree86/utils/xorgconfig

SRCS=	xorgconfig.c cards.c

LDADD=	-lxkbfile -lXext -lX11
DPADD=	${LIBXKBFILE} ${LIBXEXT} ${LIBX11}

CPPFLAGS+=	-DCARD_DATABASE_FILE='"${X11USRLIBDIR}/X11/Cards"' \
		-DXKB_RULES_DIR=\"${X11USRLIBDIR}/X11/xkb/rules\" \
		-DXVERSION="${XVERSION}" \
		-DWSCONS_SUPPORT -D__XKBDEFRULES__='"xorg"' \
		-D__XCONFIGFILE__='"xorg.conf"' \
		-D__XSERVERNAME__='"Xorg"' \
		-D__XKBDEFRULES__='"xorg"' \
		-DXVENDORNAME='"The X.Org Foundation"' \
		-DFILEMANSUFFIX=\"5\" \
		-DXVERSIONSTRING=\"1.4.2\"

.if ${MACHINE} == "i386"
CPPFLAGS+=	-DPCVT_SUPPORT
.endif

.include <bsd.x11.mk>
XVERSION!=	${PRINTX11VERSION}
.include <bsd.prog.mk>
