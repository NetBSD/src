#	$NetBSD: Makefile,v 1.1.14.1 2010/04/21 05:24:44 matt Exp $

LIB=	wfb

SRCS=	fbmodule.c fbcmap_mi.c

CPPFLAGS+=	-DFB_ACCESS_WRAPPER \
		-I${X11SRCDIR.xorg-server}/Xext \
		-I${X11SRCDIR.xorg-server}/fb

.include "../Makefile.dixmod"

.PATH:	${X11SRCDIR.xorg-server}/fb
