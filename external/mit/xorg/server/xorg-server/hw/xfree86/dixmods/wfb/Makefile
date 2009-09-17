#	$NetBSD: Makefile,v 1.1.8.1 2009/09/17 04:24:39 snj Exp $

LIB=	wfb

SRCS=	fbmodule.c fbcmap_mi.c

CPPFLAGS+=	-DFB_ACCESS_WRAPPER \
		-I${X11SRCDIR.xorg-server}/Xext \
		-I${X11SRCDIR.xorg-server}/fb

.include "../Makefile.dixmod"

.PATH:	${X11SRCDIR.xorg-server}/fb
