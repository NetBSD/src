#	$NetBSD: Makefile,v 1.5 2008/09/05 05:20:39 lukem Exp $

NOMAN=	1

.include <bsd.own.mk>

.PATH:	${X11SRCDIR.xc}/programs/rgb
PROG=		rgb
SRCS=		rgb.c
RGBSRCDIR=	${X11SRCDIR.xc}/programs/${PROG}

CPPFLAGS+=-DNDBM -DRGB_DB=\"${X11LIBDIR}/rgb\"
CPPFLAGS+=-I${X11SRCDIR.xc}/programs/Xserver/include

FILES=	rgb.txt rgb.db
FILESDIR=${X11LIBDIR}

CLEANFILES+=rgb.db
realall: rgb.db

rgb.db: ${RGBSRCDIR}/rgb.txt rgb_mkdb.sh
	${_MKTARGET_CREATE}
	rm -f ${.TARGET}
	TOOL_AWK=${AWK:Q} TOOL_DB=${TOOL_DB:Q} ${HOST_SH} \
	    ${.CURDIR}/rgb_mkdb.sh ${RGBSRCDIR}/rgb.txt ${.TARGET} \
	    || rm -f ${.TARGET}

.include <bsd.x11.mk>
.include <bsd.prog.mk>
