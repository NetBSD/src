#	$NetBSD: Makefile,v 1.9 2008/10/25 22:27:39 apb Exp $

.include <bsd.own.mk>

SUBDIR=	cache config internal

.PATH:	${X11SRCDIR.xc}/extras/freetype2/include/freetype

INCSDIR=${X11INCDIR}/freetype2/freetype

INCS=	freetype.h ftbbox.h ftbdf.h ftcache.h ftchapters.h fterrdef.h \
	fterrors.h ftglyph.h ftgzip.h ftimage.h ftincrem.h ftlist.h \
	ftlzw.h ftmac.h ftmm.h ftmoderr.h ftmodapi.h ftoutln.h ftpfr.h \
	ftrender.h ftsizes.h ftsnames.h ftstroke.h ftsynth.h ftsysio.h \
	ftsysmem.h ftsystem.h fttrigon.h fttypes.h ftwinfnt.h ftxf86.h \
	t1tables.h ttnameid.h tttables.h tttags.h ttunpat.h

SCRIPTS=	freetype-config

CLEANFILES+=	${SCRIPTS}

# XXX hijack config-subst from Xft
CONFIG_SUBST=	${TOOL_SED} -e 's%@prefix@%${X11ROOTDIR}%' \
		-e 's%@exec_prefix@%${X11BINDIR}%' \
		-e 's%@libdir@%${X11USRLIBDIR}%g' \
		-e 's%@includedir@%${X11INCDIR}%' \
		-e 's%@ft_version@%9.0.3%' \
		-e 's%@hardcode_libdir_flag_spec@%-Wl,-rpath,${X11USRLIBDIR}%' \
		-e 's%@LIBZ@%-lz%' \

freetype-config:
	${CONFIG_SUBST} < ${X11SRCDIR.xc}/extras/freetype2/builds/unix/freetype-config.in > ${.TARGET}

.include <bsd.x11.mk>
.include <bsd.prog.mk>
.include <bsd.subdir.mk>
