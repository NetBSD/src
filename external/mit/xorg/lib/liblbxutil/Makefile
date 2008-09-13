#	$NetBSD: Makefile,v 1.3 2008/09/13 04:32:31 cube Exp $

.include <bsd.own.mk>

.include "${NETBSDSRCDIR}/external/mit/xorg/tools/mkg3states/Makefile.mkg3states"

LIB=	lbxutil
.PATH:	${X11SRCDIR.${LIB}}/src/delta
.PATH:	${X11SRCDIR.${LIB}}/src/lbx_zlib
.PATH:	${X11SRCDIR.${LIB}}/src/image

SRCS=	lbx_zlib_io.c lbx_zlib.c reqstats.c \
	misc.c dpackbits.c epackbits.c dfaxg42d.c efaxg42d.c \
	lbxdelta.c

dfaxg42d.c: g3states.h
g3states.h: ${X11SRCDIR.lbxutil}/src/image/mkg3states.c
	${_MKTARGET_CREATE}
	${MKG3STATES} -c > ${.TARGET}.tmp && mv ${.TARGET}.tmp ${.TARGET}

CLEANFILES+=	g3states.h

CPPFLAGS+=	${X11FLAGS.THREADLIB} -I.

LIBDPLIBS=z ${.CURDIR}/../../../../../lib/libz

PKGCONFIG=	lbxutil

.include <bsd.x11.mk>
.include <bsd.lib.mk>
