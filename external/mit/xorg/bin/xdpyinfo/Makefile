#	$NetBSD: Makefile,v 1.3 2010/05/22 02:40:40 mrg Exp $

.include <bsd.own.mk>

# XXX add composite lib support

PROG=	xdpyinfo

CPPFLAGS+=-DMITSHM -DXKB -DXINPUT -DXRENDER
CPPFLAGS+=-DPANORAMIX -DXF86VIDMODE -DXFreeXDGA -DXF86MISC

LDADD+=	-lXtst -lXi -lXrender -lXinerama \
	-lXxf86vm -lXxf86dga -lXxf86misc -lXext -lX11
DPADD+= ${LIBXTST} ${LIBXI} ${LIBXRENDER} ${LIBXINERAMA} \
	${LIBXXF86VM} ${LIBXXF86DGA} ${LIBXEXT} ${LIBX11}

.PATH:	${X11SRCDIR.${PROG}}

COPTS.xdpyinfo.c+=	-Wno-error	# xf86vmstr.h

.include <bsd.x11.mk>
.include <bsd.prog.mk>
