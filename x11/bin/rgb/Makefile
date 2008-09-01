#	$NetBSD: Makefile,v 1.3 2008/09/01 10:37:19 rtr Exp $

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

rgb.db: ${RGBSRCDIR}/rgb.txt
	${RGB} rgb < ${RGBSRCDIR}/rgb.txt


.include "${NETBSDSRCDIR}/x11/tools/rgb/Makefile.rgb"
.include <bsd.x11.mk>
.include <bsd.prog.mk>
