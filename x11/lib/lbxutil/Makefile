#	$NetBSD: Makefile,v 1.6 2004/02/09 04:30:05 lukem Exp $

NOPIC=	1

.include <bsd.own.mk>

LIB=	lbxutil

.PATH:	${X11SRCDIR.xc}/lib/${LIB}/lbx_zlib
SRCS=	lbx_zlib.c lbx_zlib_io.c reqstats.c

.PATH:	${X11SRCDIR.xc}/lib/${LIB}/delta
SRCS+=	lbxdelta.c

.PATH:	${X11SRCDIR.xc}/lib/${LIB}/image
SRCS+=	dfaxg42d.c dpackbits.c efaxg42d.c epackbits.c misc.c

CPPFLAGS+=	-I${DESTDIR}${X11INCDIR}/X11 \
		-I${DESTDIR}${X11INCDIR}/X11/extensions \
		-I${X11SRCDIR.xc}/programs/Xserver/include \
		-I.

.include "${NETBSDSRCDIR}/x11/tools/mkg3states/Makefile.mkg3states"

g3states.h:
	${_MKTARGET_CREATE}
	rm -f ${.TARGET}
	${MKG3STATES} > ${.TARGET}
CLEANFILES+=	g3states.h

dfaxg42d.c: g3states.h

.include <bsd.x11.mk>
.include <bsd.lib.mk>
