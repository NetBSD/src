#	$NetBSD: Makefile,v 1.2 2004/03/06 03:31:06 rtr Exp $

.include "../Makefile.serverlib"

.include <bsd.own.mk>

LIB=	fontbase


# fontfile
.PATH:		${X11SRCDIR.xc}/lib/font/fontfile
SRCS.fontfile=	dirfile.c fontdir.c fontfile.c fileio.c fontscale.c \
		defaults.c bitsource.c register.c renderers.c bufio.c \
		decompress.c filewr.c printerfont.c ffcheck.c \
		fontenc.c encparse.c fontencc.c gunzip.c
.for fn in ${SRCS.fontfile}
CPPFLAGS.${fn}= -I${X11SRCDIR.xc}/include/fonts \
                -I${X11SRCDIR.xc}/lib/font/include \
                -I${X11SRCDIR.xc}/programs/Xserver/include \
                -DFONTDIRATTRIB -DBUILD_SPEEDO -DBUILD_TYPE1 -DBUILD_CID \
                -DBUILD_FREETYPE -DX_GZIP_FONT_COMPRESSION \
                -DFONT_ENCODINGS_DIRECTORY=\"${X11FONTDIR}/encodings/encodings.dir\"
.endfor
CPPFLAGS.ffcheck.c+=	-DLOADABLEFONTS
CPPFLAGS.register.c+=	-DLOADABLEFONTS


# fc 
.PATH:		${X11SRCDIR.xc}/lib/font/fc
SRCS.fc=	fsconvert.c fserve.c fsio.c
.PATH:          ${X11SRCDIR.xc}/lib/xtrans
SRCS.fc+=	transport.c
.for fn in ${SRCS.fc}
CPPFLAGS.${fn}= -I${X11SRCDIR.xc}/include/fonts \
                -I${X11SRCDIR.xc}/lib/font/include \
		-I${X11SRCDIR.xc}/lib/xtrans \
                ${X11FLAGS.CONNECTION} \
                -DFONT_t -DTRANS_CLIENT -DTRANS_SERVER -DTRANS_REOPEN \
                -DBSD44SOCKETS
.endfor


# fontcache
.PATH:          ${X11SRCDIR.xc}/lib/font/fontcache
SRCS.fontcache= fontcache.c
.for fn in ${SRCS.fontcache}
CPPFLAGS.${fn}= -I${X11SRCDIR.xc}/lib/font/fontcache \
                -I${X11SRCDIR.xc}/include/fonts \
                -I${X11SRCDIR.xc}/lib/font/include \
                -I${X11SRCDIR.xc}/lib/font/fontcache \
                -DFONTCACHE
.endfor


# util
.PATH:          ${X11SRCDIR.xc}/lib/font/util
SRCS.util=	utilbitmap.c fontnames.c fontutil.c fontxlfd.c format.c \
		fontaccel.c atom.c miscutil.c private.c patcache.c
.for fn in ${SRCS.util}
CPPFLAGS.${fn}= -I${X11SRCDIR.xc}/include/fonts \
                -I${X11SRCDIR.xc}/lib/font/include
.endfor

SRCS=	${SRCS.fontfile} ${SRCS.fc} ${SRCS.util} ${SRCS.fontcache}

.include <bsd.x11.mk>
.include <bsd.lib.mk>
