#	$NetBSD: Makefile,v 1.1 2003/12/20 01:27:06 rtr Exp $

.include "../Makefile.serverlib"

.include <bsd.own.mk>

LIB=	lbxutil

.PATH:	${X11SRCDIR.xc}/lib/${LIB}/lbx_zlib
SRCS=	lbx_zlib.c lbx_zlib_io.c reqstats.c

.PATH:	${X11SRCDIR.xc}/lib/${LIB}/delta
SRCS+=	lbxdelta.c

.PATH:	${X11SRCDIR.xc}/lib/${LIB}/image
SRCS+=	dfaxg42d.c dpackbits.c efaxg42d.c epackbits.c misc.c

CPPFLAGS+=	-I${DESTDIR}${X11INCDIR}/X11 \
		-I${DESTDIR}${X11INCDIR}/X11/extensions \
		-I${X11SRCDIR.xc}/programs/Xserver/include \
		-I.

.include <bsd.x11.mk>
.include <bsd.lib.mk>
