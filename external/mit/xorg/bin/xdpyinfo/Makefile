#	$NetBSD: Makefile,v 1.7.24.1 2021/06/06 20:30:46 cjep Exp $

.include <bsd.own.mk>

# XXX add composite lib support

PROG=	xdpyinfo

CPPFLAGS+=-I${X11SRCDIR.${PROG}}/../include -DHAVE_CONFIG_H

# XXX all libraries are +1 on X11 or +1 on XEXT
LDADD+=	-lXtst -lXi -lXrender -lXinerama -lXcomposite \
	-lXxf86vm -lXxf86dga -lXxf86misc ${XLIBLDADD_XEXT} -lX11-xcb
DPADD+= ${LIBXTST} ${LIBXI} ${LIBXRENDER} ${LIBXINERAMA} ${LIBXCOMPOSITE} \
	${LIBXXF86VM} ${LIBXXF86DGA} ${LIBXXF86MISC} ${XLIBDPADD_XEXT} ${LIBX11XCB}

.PATH:	${X11SRCDIR.${PROG}}
.PATH:	${X11SRCDIR.${PROG}}/man

COPTS.xdpyinfo.c+=	-Wno-error	# xf86vmstr.h

.include "../../xorg-pkg-ver.mk"

.include <bsd.x11.mk>
.include <bsd.prog.mk>
