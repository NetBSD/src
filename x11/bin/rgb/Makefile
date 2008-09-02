#	$NetBSD: Makefile,v 1.4 2008/09/02 01:39:19 lukem Exp $

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

CLEANFILES+=rgb.db rgb.dir rgb.pag
realall: rgb.db

rgb.db: ${RGBSRCDIR}/rgb.txt
	${_MKTARGET_CREATE}
	${RGB} rgb < ${RGBSRCDIR}/rgb.txt


.include "${NETBSDSRCDIR}/x11/tools/rgb/Makefile.rgb"
.include <bsd.x11.mk>
.include <bsd.prog.mk>
