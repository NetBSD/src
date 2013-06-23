#	$NetBSD: Makefile,v 1.3.12.1 2013/06/23 06:28:40 tls Exp $

.include <bsd.own.mk>

# XXX add composite lib support

PROG=	xdpyinfo

CPPFLAGS+=-I${X11SRCDIR.${PROG}}/../include -DHAVE_CONFIG_H

LDADD+=	-lXtst -lXi -lXrender -lXinerama -lXcomposite \
	-lXxf86vm -lXxf86dga -lXxf86misc -lXext -lX11 \
	-lX11-xcb
DPADD+= ${LIBXTST} ${LIBXI} ${LIBXRENDER} ${LIBXINERAMA} ${LIBXCOMPOSITE} \
	${LIBXXF86VM} ${LIBXXF86DGA} ${LIBXXF86MISC} ${LIBXEXT} ${LIBX11} \
	${LIBX11XCB}

.PATH:	${X11SRCDIR.${PROG}}
.PATH:	${X11SRCDIR.${PROG}}/man

COPTS.xdpyinfo.c+=	-Wno-error	# xf86vmstr.h

.include <bsd.x11.mk>
.include <bsd.prog.mk>
