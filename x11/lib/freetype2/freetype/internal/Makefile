#	$NetBSD: Makefile,v 1.3.4.1 2004/11/15 09:12:46 rtr Exp $

.include <bsd.own.mk>

.PATH:	${X11SRCDIR.xc}/extras/freetype2/include/freetype/internal

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
