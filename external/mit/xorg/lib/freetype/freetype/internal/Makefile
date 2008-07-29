#	$NetBSD: Makefile,v 1.1.1.1 2008/07/29 05:01:22 mrg Exp $

.include <bsd.own.mk>

.PATH:	${X11SRCDIR.freetype}/include/freetype/internal

INCSDIR=${X11INCDIR}/freetype2/freetype/internal

INCS=	autohint.h ftcalc.h \
	ftdebug.h ftdriver.h ftgloadr.h \
	ftmemory.h ftobjs.h ftstream.h \
	fttrace.h internal.h pcftypes.h psaux.h pshints.h \
	sfnt.h t1types.h tttypes.h ftrfork.h ftserv.h

.include <bsd.x11.mk>
.include <bsd.prog.mk>

SUBDIR=	services

.include <bsd.subdir.mk>
