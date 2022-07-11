#	$NetBSD: Makefile,v 1.5 2022/07/11 08:51:22 mrg Exp $

.include <bsd.own.mk>

PROG=	setxkbmap

CPPFLAGS+=	-DDFLT_XKB_CONFIG_ROOT=\"${X11LIBDIR}/xkb\" \
		-D__XKBDEFRULES__=${__XKBDEFRULES__}

LDADD+=	-lXrandr -lxkbfile -lXext -lX11
DPADD+=	${LIBXRANDR} ${LIBXKBFILE} ${LIBXEXT} ${LIBX11}

.PATH:	${X11SRCDIR.${PROG}}
.PATH:	${X11SRCDIR.${PROG}}/man

X11EXTRAMANDEFS+=	-e 's,__xkbconfigroot__,${X11LIBDIR}/xkb,g'

.include "../../xorg-pkg-ver.mk"

.include <bsd.x11.mk>
.include <bsd.prog.mk>
