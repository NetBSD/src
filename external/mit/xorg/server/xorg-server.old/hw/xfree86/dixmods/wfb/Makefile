#	$NetBSD: Makefile,v 1.1.1.1 2016/06/10 03:42:14 mrg Exp $

LIB=	wfb

SRCS=	fbmodule.c fbcmap_mi.c

CPPFLAGS+=	-DFB_ACCESS_WRAPPER \
		-I${X11SRCDIR.xorg-server}/Xext \
		-I${X11SRCDIR.xorg-server}/fb

.include "../Makefile.dixmod"

.PATH:	${X11SRCDIR.xorg-server}/fb
