#	$NetBSD: Makefile,v 1.1.1.1 2008/07/29 05:01:22 mrg Exp $

.include <bsd.own.mk>

PROG=	xdpyinfo

CPPFLAGS+=-DMITSHM -DXKB -DXINPUT -DXRENDER
CPPFLAGS+=-DPANORAMIX -DXF86VIDMODE -DXFreeXDGA -DXF86MISC

LDADD+=	-lXtst -lXi -lXrender -lXinerama \
	-lXxf86vm -lXxf86dga -lXxf86misc -lXext -lX11
DPADD+= ${LIBXTST} ${LIBXI} ${LIBXRENDER} ${LIBXINERAMA} \
	${LIBXXF86VM} ${LIBXXF86DGA} ${LIBXEXT} ${LIBX11}

.PATH:	${X11SRCDIR.${PROG}}

.include <bsd.x11.mk>
.include <bsd.prog.mk>
