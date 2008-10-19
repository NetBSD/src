#	$NetBSD: Makefile,v 1.1.6.2 2008/10/19 22:41:19 haad Exp $

LIB=	wfb

SRCS=	fbmodule.c fbcmap_mi.c

CPPFLAGS+=	-DFB_ACCESS_WRAPPER \
		-I${X11SRCDIR.xorg-server}/fb

.include "../Makefile.dixmod"

.PATH:	${X11SRCDIR.xorg-server}/fb
