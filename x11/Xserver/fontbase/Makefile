#	$NetBSD: Makefile,v 1.1 2003/12/23 03:36:44 rtr Exp $

.include "../Makefile.serverlib"

.include <bsd.own.mk>

LIB=	fontbase

SRCS.fontfile=  bitsource.c bufio.c decompress.c defaults.c dirfile.c \
                encparse.c ffcheck.c fileio.c filewr.c fontdir.c \
                fontenc.c fontencc.c fontfile.c fontscale.c gunzip.c \
                printerfont.c register.c renderers.c
.PATH:          ${X11SRCDIR.xc}/lib/font/fontfile
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

SRCS.fc=        fsconvert.c fserve.c fsio.c transport.c
.PATH:          ${X11SRCDIR.xc}/lib/font/fc
.PATH:          ${X11SRCDIR.xc}/lib/xtrans
.for fn in ${SRCS.fc}
CPPFLAGS.${fn}= -I${X11SRCDIR.xc}/include/fonts \
                -I${X11SRCDIR.xc}/lib/font/include \
		-I${X11SRCDIR.xc}/lib/xtrans \
                ${X11FLAGS.CONNECTION} \
                -DFONT_t -DTRANS_CLIENT -DTRANS_SERVER -DTRANS_REOPEN \
                -DBSD44SOCKETS
.endfor

SRCS.fontcache= fontcache.c
.PATH:          ${X11SRCDIR.xc}/lib/font/fontcache
.for fn in ${SRCS.fontcache}
CPPFLAGS.${fn}= -I${X11SRCDIR.xc}/lib/font/fontcache \
                -I${X11SRCDIR.xc}/include/fonts \
                -I${X11SRCDIR.xc}/lib/font/include \
                -I${X11SRCDIR.xc}/lib/font/fontcache \
                -DFONTCACHE
.endfor


SRCS.util=      atom.c fontaccel.c fontnames.c fontutil.c fontxlfd.c \
                format.c miscutil.c patcache.c private.c utilbitmap.c
.PATH:          ${X11SRCDIR.xc}/lib/font/util
.for fn in ${SRCS.util}
CPPFLAGS.${fn}= -I${X11SRCDIR.xc}/include/fonts \
                -I${X11SRCDIR.xc}/lib/font/include
.endfor

SRCS=	${SRCS.fontfile} ${SRCS.fc} ${SRCS.util} ${SRCS.fontcache}

.include <bsd.x11.mk>
.include <bsd.lib.mk>
