#	$NetBSD: Makefile,v 1.4.4.1 2004/11/21 10:01:27 rtr Exp $

.include "../Makefile.serverlib"
.include "../Makefile.servermod"

LIB=		lbx

.PATH:		${X11SRCDIR.xc}/programs/Xserver/lbx
SRCS=		lbxcmap.c lbxdix.c lbxexts.c lbxgfx.c lbxmain.c lbxopts.c \
		lbxprop.c lbxsquish.c lbxswap.c lbxtables.c lbxtags.c \
		lbxzerorep.c

CPPFLAGS+=	-I${DESTDIR}${X11INCDIR}/X11 \
		-I${X11SRCDIR.xc}/include/fonts \
		-I${X11SRCDIR.xc}/programs/Xserver/include \
		-I${DESTDIR}${X11INCDIR}/X11/extensions

COPTS.lbxgfx.c=	-Wno-error	# XXX gcc 3.3

.include <bsd.x11.mk>
.include <bsd.lib.mk>
