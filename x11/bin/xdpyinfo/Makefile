#	$NetBSD: Makefile,v 1.1.1.1 2003/09/11 18:37:07 lukem Exp $

.include <bsd.own.mk>

PROG=	xdpyinfo

CPPFLAGS+=-DMITSHM -DXKB -DXINPUT -DXRENDER
CPPFLAGS+=-DPANORAMIX -DXF86VIDMODE -DXFreeXDGA -DXF86MISC

LDADD+=	-lXtst -lXi -lXrender -lXinerama \
	-lXxf86vm -lXxf86dga -lXxf86misc -lXext -lX11
DPADD+= ${LIBXTST} ${LIBXI} ${LIBXRENDER} ${LIBXINERAMA} \
	${LIBXXF86VM} ${LIBXXF86DGA} ${LIBXEXT} ${LIBX11}

.PATH:	${X11SRCDIR.xc}/programs/${PROG}

.include <bsd.prog.mk>
.include <bsd.x11.mk>
