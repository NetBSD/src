#	$NetBSD: Makefile,v 1.1 2018/03/14 08:55:35 mrg Exp $

.include <bsd.own.mk>

.PATH:  ${X11SRCDIR.xorgproto}/include/GL

INCS=	glxint.h \
	glxmd.h \
	glxproto.h \
	glxtokens.h
INCSDIR=${X11INCDIR}/GL

SUBDIR=	internal

NOOBJ=  # defined

.include <bsd.subdir.mk>
.include <bsd.x11.mk>
.include <bsd.prog.mk>
