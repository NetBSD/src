#	$NetBSD: Makefile,v 1.1.1.1 2003/09/11 18:37:05 lukem Exp $

.include <bsd.own.mk>

LIB=		lbx
.PATH:		${X11SRCDIR.xc}/programs/Xserver/lbx

SRCS=		lbxcmap.c lbxdix.c lbxexts.c lbxgfx.c lbxmain.c lbxopts.c \
		lbxprop.c lbxsquish.c lbxswap.c lbxtables.c lbxtags.c \
		lbxzerorep.c

CPPFLAGS+=	-I${DESTDIR}/${X11INCDIR}/X11 \
		-I${X11SRCDIR.xc}/include/fonts \
		-I${X11SRCDIR.xc}/programs/Xserver/include \
		-I${DESTDIR}/${X11INCDIR}/X11/extensions

.include <bsd.lib.mk>
.include <bsd.x11.mk>
