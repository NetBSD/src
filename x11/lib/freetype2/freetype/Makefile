#	$NetBSD: Makefile,v 1.3 2004/01/06 06:15:45 rtr Exp $

.include <bsd.own.mk>

SUBDIR=	cache config internal

.PATH:	${X11SRCDIR.xc}/extras/freetype2/include/freetype

INCSDIR=${X11INCDIR}/freetype2/freetype

INCS=	ftbdf.h fterrdef.h ftsysio.h ftsysmem.h ftxf86.h freetype.h \
	ftbbox.h ftcache.h ftchapters.h fterrors.h ftglyph.h \
	ftimage.h ftlist.h ftmac.h ftmm.h ftmoderr.h ftmodule.h \
	ftoutln.h ftrender.h ftsizes.h ftsnames.h ftsynth.h ftsystem.h \
	fttrigon.h fttypes.h t1tables.h ttnameid.h tttables.h tttags.h

SCRIPTS=	freetype-config

.PHONY:		freetype-config
all dependall:	freetype-config

# XXX hijack config-subst from Xft
CONFIG_SUBST=	sed -e 's%@prefix@%${X11ROOTDIR}%' \
		-e 's%@exec_prefix@%${X11BINDIR}%' \
		-e 's%@libdir@%${X11USRLIBDIR}%g' \
		-e 's%@includedir@%${X11INCDIR}%' \
		-e 's%@ft_version@%9.0.3%'

freetype-config:
	${CONFIG_SUBST} < ${X11SRCDIR.xc}/extras/freetype2/builds/unix/freetype-config.in > ${.TARGET}

.include <bsd.x11.mk>
.include <bsd.prog.mk>
.include <bsd.subdir.mk>
