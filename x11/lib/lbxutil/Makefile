#	$NetBSD: Makefile,v 1.1.1.1 2003/09/11 18:37:09 lukem Exp $

NOPIC=	1

.include <bsd.own.mk>

LIB=	lbxutil

SUBDIR=	mkg3states


.PATH:	${X11SRCDIR.xc}/lib/${LIB}/lbx_zlib
SRCS=	lbx_zlib.c lbx_zlib_io.c reqstats.c

.PATH:	${X11SRCDIR.xc}/lib/${LIB}/delta
SRCS+=	lbxdelta.c

.PATH:	${X11SRCDIR.xc}/lib/${LIB}/image
SRCS+=	dfaxg42d.c dpackbits.c efaxg42d.c epackbits.c misc.c

CPPFLAGS+=	-I${DESTDIR}/${X11INCDIR}/X11 \
		-I${DESTDIR}/${X11INCDIR}/X11/extensions \
		-I${X11SRCDIR.xc}/programs/Xserver/include \
		-I.

MKG3STATESDIR!=	cd ${.CURDIR}/mkg3states && ${PRINTOBJDIR}
MKG3STATES=	${MKG3STATESDIR}/mkg3states

${MKG3STATES}:
	(cd ${.CURDIR}/mkg3states && ${MAKE})

g3states.h: ${MKG3STATES}
	rm -f ${.TARGET}
	${MKG3STATES} > ${.TARGET}
CLEANFILES+=	g3states.h

dfaxg42d.c: g3states.h

.include <bsd.lib.mk>
.include <bsd.subdir.mk>
.include <bsd.x11.mk>
