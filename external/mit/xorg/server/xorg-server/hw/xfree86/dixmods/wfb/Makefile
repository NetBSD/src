#	$NetBSD: Makefile,v 1.1.2.2 2008/09/18 05:18:45 wrstuden Exp $

LIB=	wfb

SRCS=	fbmodule.c fbcmap_mi.c

CPPFLAGS+=	-DFB_ACCESS_WRAPPER \
		-I${X11SRCDIR.xorg-server}/fb

.include "../Makefile.dixmod"

.PATH:	${X11SRCDIR.xorg-server}/fb
