#	$NetBSD: Makefile,v 1.1.1.1 2003/09/11 18:37:09 lukem Exp $

.include <bsd.own.mk>

SUBDIR=	cache config internal

.PATH:	${X11SRCDIR.xc}/extras/freetype2/include/freetype

INCSDIR=${X11INCDIR}/freetype2/freetype

INCS=	ftbdf.h fterrdef.h ftsysio.h ftsysmem.h ftxf86.h freetype.h \
	ftbbox.h ftcache.h ftchapters.h fterrors.h ftglyph.h \
	ftimage.h ftlist.h ftmac.h ftmm.h ftmoderr.h ftmodule.h \
	ftoutln.h ftrender.h ftsizes.h ftsnames.h ftsynth.h ftsystem.h \
	fttrigon.h fttypes.h t1tables.h ttnameid.h tttables.h tttags.h

.include <bsd.prog.mk>
.include <bsd.subdir.mk>
.include <bsd.x11.mk>
