#	$NetBSD: Makefile,v 1.2 2008/10/14 23:37:19 cube Exp $

.include <bsd.own.mk>

PROG=	setxkbmap

CPPFLAGS+=	-DDFLT_XKB_CONFIG_ROOT=\"${X11LIBDIR}/xkb\" \
		-D__XKBDEFRULES__=${__XKBDEFRULES__}

LDADD+=	-lxkbfile -lXext -lX11
DPADD+=	${LIBXKBFILE} ${LIBXEXT} ${LIBX11}

.PATH:	${X11SRCDIR.${PROG}}

X11EXTRAMANDEFS+=	-e 's,__xkbconfigroot__,${X11LIBDIR}/xkb,g'

.include <bsd.x11.mk>
.include <bsd.prog.mk>
