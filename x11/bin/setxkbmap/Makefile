#	$NetBSD: Makefile,v 1.2.4.1 2004/11/15 09:12:45 rtr Exp $

.include <bsd.own.mk>

PROG=	setxkbmap

CPPFLAGS+=	-DDFLT_XKB_CONFIG_ROOT=\"${X11LIBDIR}/xkb\" \
		-D__XKBDEFRULES__=${__XKBDEFRULES__}

LDADD+=	-lxkbfile -lXext -lX11
DPADD+=	${LIBXKBFILE} ${LIBXEXT} ${LIBX11}

.PATH:	${X11SRCDIR.xc}/programs/${PROG}

.include <bsd.x11.mk>
.include <bsd.prog.mk>
