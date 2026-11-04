#	$NetBSD: Makefile,v 1.8.2.1 2026/11/04 23:15:24 perseant Exp $

.include <bsd.own.mk>

# XXX add composite lib support

PROG=	xdpyinfo

CPPFLAGS+=-I${X11SRCDIR.${PROG}}/../include -DHAVE_CONFIG_H

LDADD+=	-lXtst -lXi -lXrender -lXinerama -lXcomposite \
	-lXpresent -lXrandr \
	-lXxf86vm -lXxf86dga -lXext -lX11 \
	-lxcb -lX11-xcb
DPADD+= ${LIBXTST} ${LIBXI} ${LIBXRENDER} ${LIBXINERAMA} ${LIBXCOMPOSITE} \
	${LIBXPRESENT} ${LIBXRANDR} \
	${LIBXXF86VM} ${LIBXXF86DGA} ${LIBXEXT} ${LIBX11} \
	${LIBXCB} ${LIBX11XCB}

.PATH:	${X11SRCDIR.${PROG}}
.PATH:	${X11SRCDIR.${PROG}}/man

COPTS.xdpyinfo.c+=	-Wno-error	# xf86vmstr.h

.include "../../xorg-pkg-ver.mk"

.include <bsd.x11.mk>
.include <bsd.prog.mk>
