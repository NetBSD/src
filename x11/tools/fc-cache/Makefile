#	$NetBSD: Makefile,v 1.8.2.1 2004/11/15 09:12:46 rtr Exp $

NOMAN=		1

.include <bsd.own.mk>

HOSTPROG=	fc-cache

FONTCONFIG=	${X11SRCDIR.xc}/extras/fontconfig
FREETYPE=	${X11SRCDIR.xc}/extras/freetype2
EXPAT=		${X11SRCDIR.xc}/extras/expat


.PATH:	${FONTCONFIG}/fc-cache
SRCS.fc-cache=	fc-cache.c


.PATH:	${FONTCONFIG}/src
SRCS.fontconfig=	fcatomic.c fcblanks.c fccache.c fccfg.c \
		fccharset.c fcdbg.c fcdefault.c fcdir.c fcfreetype.c \
		fcfs.c fcinit.c fclang.c fclist.c fcmatch.c fcmatrix.c \
		fcname.c fcpat.c fcstr.c fcxml.c

.PATH:	${FREETYPE}/src/base
SRCS.freetype=	ftapi.c ftbase.c ftbbox.c ftbdf.c ftdebug.c ftglyph.c \
		ftinit.c ftmm.c ftpfr.c ftstroke.c ftsynth.c ftsystem.c \
		fttype1.c ftwinfnt.c ftxf86.c

.PATH: ${FREETYPE}/src/autofit
SRCS.freetype+=	autofit.c

.PATH:	${FREETYPE}/src/autohint
SRCS.freetype+=	autohint.c

.PATH:	${FREETYPE}/src/bdf
SRCS.freetype+=	bdf.c

.PATH:	${FREETYPE}/src/cff
SRCS.freetype+=	cff.c

.PATH:	${FREETYPE}/src/cid
SRCS.freetype+=	type1cid.c

.PATH:	${FREETYPE}/src/gzip
SRCS.freetype+=	ftgzip.c

.PATH:	${FREETYPE}/src/lzw
SRCS.freetype+=	ftlzw.c

.PATH:	${FREETYPE}/src/pcf
SRCS.freetype+=	pcf.c

.PATH:	${FREETYPE}/src/pfr
SRCS.freetype+=	pfr.c

.PATH:	${FREETYPE}/src/psaux
SRCS.freetype+=	psaux.c

.PATH:	${FREETYPE}/src/pshinter
SRCS.freetype+=	pshinter.c

.PATH:	${FREETYPE}/src/psnames
SRCS.freetype+=	psnames.c

.PATH:	${FREETYPE}/src/raster
SRCS.freetype+=	raster.c

.PATH:	${FREETYPE}/src/sfnt
SRCS.freetype+=	sfnt.c

.PATH:	${FREETYPE}/src/smooth
SRCS.freetype+=	smooth.c

.PATH:	${FREETYPE}/src/truetype
SRCS.freetype+=	truetype.c

.PATH:	${FREETYPE}/src/type1
SRCS.freetype+=	type1.c

.PATH:	${FREETYPE}/src/type42
SRCS.freetype+=	type42.c

.PATH:	${FREETYPE}/src/winfonts
SRCS.freetype+=	winfnt.c


.PATH:	${EXPAT}/lib
SRCS.expat=	xmlparse.c xmltok.c xmlrole.c

SRCS=	${SRCS.fc-cache} ${SRCS.fontconfig} ${SRCS.freetype} ${SRCS.expat}

# XXX FONTCONFIG_PATH was arbitrarily chosen
HOST_CPPFLAGS=	-DFONTCONFIG_PATH='"/etc/X11/fonts"' \
		-DFC_DEFAULT_FONTS='"${X11FONTDIR}/Type1"'
HOST_CPPFLAGS+=	-I${FONTCONFIG} -I${FREETYPE} \
		-I${FREETYPE}/include -I${EXPAT}/lib \
		-I${X11SRCDIR.xc}/lib/expat \
		-I${DESTDIR}${X11INCDIR}


.include <bsd.x11.mk>
.include <bsd.hostprog.mk>
