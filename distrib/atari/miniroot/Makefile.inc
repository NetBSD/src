#	$NetBSD: Makefile.inc,v 1.8 2002/03/28 04:33:21 lukem Exp $

IMAGESIZE=	1080k
IMAGEENDIAN=	be
MAKEDEVTARGETS=	floppy
IMAGEDEPENDS+=	${ARCHDIR}/disktab.shadow ${ARCHDIR}/dot.profile \
		${ARCHDIR}/install.md ${ARCHDIR}/termcap.vt

# Use stubs to eliminate some large stuff from libc
HACKSRC=	${DISTRIBDIR}/utils/libhack
HACKOBJS=	gethost.o
.include	"${HACKSRC}/Makefile.inc"
${CRUNCHBIN}:	libhack.o
