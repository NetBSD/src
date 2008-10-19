#	$NetBSD: Makefile,v 1.1.1.1.6.2 2008/10/19 22:41:00 haad Exp $

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
