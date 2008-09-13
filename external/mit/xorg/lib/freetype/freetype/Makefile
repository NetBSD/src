#	$NetBSD: Makefile,v 1.4 2008/09/13 20:51:34 cube Exp $

.include <bsd.own.mk>

SUBDIR=	config internal

.PATH:	${X11SRCDIR.freetype}/include/freetype

INCSDIR=${X11INCDIR}/freetype2/freetype

INCS=	freetype.h ftbbox.h ftbdf.h ftcache.h ftchapters.h fterrdef.h \
	fterrors.h ftglyph.h ftgzip.h ftimage.h ftincrem.h ftlist.h \
	ftmac.h ftmm.h ftmoderr.h ftoutln.h ftpfr.h \
	ftrender.h ftsizes.h ftsnames.h ftsynth.h \
	ftsystem.h fttrigon.h fttypes.h ftwinfnt.h ftxf86.h \
	t1tables.h ttnameid.h tttables.h tttags.h ftmodapi.h ftstroke.h \
	ftlzw.h ttunpat.h

SCRIPTS=	freetype-config

CLEANFILES+=	${SCRIPTS}

# XXX hijack config-subst from Xft
CONFIG_SUBST=	sed -e 's%@prefix@%${X11ROOTDIR}%' \
		-e 's%@exec_prefix@%${X11BINDIR}%' \
		-e 's%@libdir@%${X11USRLIBDIR}%g' \
		-e 's%@includedir@%${X11INCDIR}%' \
		-e 's%@ft_version@%9.17.3%' \
		-e 's%@FT2_EXTRA_LIBS@%%' \
		-e 's%@hardcode_libdir_flag_spec@%-Wl,-rpath,${X11USRLIBDIR}%' \
		-e 's%@LIBZ@%-lz%' \

freetype-config:
	${CONFIG_SUBST} < ${X11SRCDIR.freetype}/builds/unix/freetype-config.in > ${.TARGET}

.include <bsd.x11.mk>
.include <bsd.prog.mk>
.include <bsd.subdir.mk>
