#	$NetBSD: Makefile,v 1.8 2024/05/09 06:34:50 nia Exp $

.include <bsd.own.mk>

# XXX add composite lib support

PROG=	xdpyinfo

CPPFLAGS+=-I${X11SRCDIR.${PROG}}/../include -DHAVE_CONFIG_H

LDADD+=	-lXtst -lXi -lXrender -lXinerama -lXcomposite \
	-lXxf86vm -lXxf86dga -lXext -lX11 \
	-lxcb -lX11-xcb
DPADD+= ${LIBXTST} ${LIBXI} ${LIBXRENDER} ${LIBXINERAMA} ${LIBXCOMPOSITE} \
	${LIBXXF86VM} ${LIBXXF86DGA} ${LIBXEXT} ${LIBX11} \
	${LIBXCB} ${LIBX11XCB}

.PATH:	${X11SRCDIR.${PROG}}
.PATH:	${X11SRCDIR.${PROG}}/man

COPTS.xdpyinfo.c+=	-Wno-error	# xf86vmstr.h

.include "../../xorg-pkg-ver.mk"

.include <bsd.x11.mk>
.include <bsd.prog.mk>
