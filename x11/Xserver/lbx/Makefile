#	$NetBSD: Makefile,v 1.3 2003/09/30 06:41:29 lukem Exp $

.include "../Makefile.serverlib"

.include <bsd.own.mk>

LIB=		lbx
.PATH:		${X11SRCDIR.xc}/programs/Xserver/lbx

SRCS=		lbxcmap.c lbxdix.c lbxexts.c lbxgfx.c lbxmain.c lbxopts.c \
		lbxprop.c lbxsquish.c lbxswap.c lbxtables.c lbxtags.c \
		lbxzerorep.c

CPPFLAGS+=	-I${DESTDIR}${X11INCDIR}/X11 \
		-I${X11SRCDIR.xc}/include/fonts \
		-I${X11SRCDIR.xc}/programs/Xserver/include \
		-I${DESTDIR}${X11INCDIR}/X11/extensions

.include <bsd.x11.mk>
.include <bsd.lib.mk>
