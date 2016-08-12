#	$NetBSD: Makefile,v 1.3 2016/08/12 01:15:50 mrg Exp $

LIB=	wfb

SRCS=	fbmodule.c

CPPFLAGS+=	-DFB_ACCESS_WRAPPER \
		-I${X11SRCDIR.xorg-server}/Xext \
		-I${X11SRCDIR.xorg-server}/fb

.include "../Makefile.dixmod"

.PATH:	${X11SRCDIR.xorg-server}/fb
