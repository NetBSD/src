#	$NetBSD: Makefile,v 1.2 2003/09/13 20:19:08 lukem Exp $

NOMAN=		1

.include <bsd.own.mk>

HOSTPROG=	fc-cache
FONTCONFIGSRC=	${X11SRCDIR.xc}/lib/fontconfig
FREETYPESRC=	${X11SRCDIR.xc}/extras/freetype2/src

.PATH:		${FONTCONFIGSRC}/${HOSTPROG}
SRCS=		fc-cache.c

.PATH:		${FONTCONFIGSRC}/src
SRCS+=		fcatomic.c fcblanks.c fccache.c fccfg.c fccharset.c fcdbg.c \
		fcdefault.c fcdir.c fcfreetype.c fcfs.c fcinit.c fclang.c \
		fclist.c fcmatch.c fcmatrix.c fcname.c fcpat.c fcstr.c fcxml.c

.PATH:		${FREETYPESRC}/autohint
.PATH:		${FREETYPESRC}/base
.PATH:		${FREETYPESRC}/bdf
.PATH:		${FREETYPESRC}/cff
.PATH:		${FREETYPESRC}/cid
.PATH:		${FREETYPESRC}/pcf
.PATH:		${FREETYPESRC}/pfr
.PATH:		${FREETYPESRC}/psaux
.PATH:		${FREETYPESRC}/pshinter
.PATH:		${FREETYPESRC}/psnames
.PATH:		${FREETYPESRC}/raster
.PATH:		${FREETYPESRC}/sfnt
.PATH:		${FREETYPESRC}/smooth
.PATH:		${FREETYPESRC}/truetype
.PATH:		${FREETYPESRC}/type1
.PATH:		${FREETYPESRC}/type42
.PATH:		${FREETYPESRC}/winfonts
SRCS+=		autohint.c bdf.c cff.c ftbase.c ftbbox.c ftbdf.c ftdebug.c \
		ftglyph.c ftinit.c ftmm.c ftsynth.c ftsystem.c fttype1.c \
		ftxf86.c pcf.c pfr.c psaux.c pshinter.c psnames.c raster.c \
		sfnt.c smooth.c truetype.c type1.c type1cid.c type42.c winfnt.c

.PATH:		${X11SRCDIR.xc}/extras/expat/lib
SRCS+=		xmlparse.c xmlrole.c xmltok.c

.include "../../lib/expat/Makefile.expatdefs"

HOST_CPPFLAGS+=	-I${DESTDIR}/${X11INCDIR} \
		-I${DESTDIR}/${X11INCDIR}/freetype2 \
		-I${X11SRCDIR.xc}/lib/expat \
		-DFC_DEFAULT_FONTS='"${X11FONTDIR}/Type1"' -DHAVE_EXPAT \
		${EXPAT_FLAGS}

.include <bsd.x11.mk>
.include <bsd.hostprog.mk>
