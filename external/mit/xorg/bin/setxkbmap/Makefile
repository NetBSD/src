#	$NetBSD: Makefile,v 1.4.24.1 2021/06/02 14:46:19 cjep Exp $

.include <bsd.own.mk>

PROG=	setxkbmap

CPPFLAGS+=	-DDFLT_XKB_CONFIG_ROOT=\"${X11LIBDIR}/xkb\" \
		-D__XKBDEFRULES__=${__XKBDEFRULES__}

LDADD+=	${XLIBLDADD_KDFILE}
DPADD+=	${XLIBDPADD_KDFILE}

.PATH:	${X11SRCDIR.${PROG}}
.PATH:	${X11SRCDIR.${PROG}}/man

X11EXTRAMANDEFS+=	-e 's,__xkbconfigroot__,${X11LIBDIR}/xkb,g'

.include "../../xorg-pkg-ver.mk"

.include <bsd.x11.mk>
.include <bsd.prog.mk>
