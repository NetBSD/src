#	$NetBSD: Makefile.inc,v 1.6 2002/03/06 12:04:50 lukem Exp $

IMAGESIZE=	1080k
IMAGEENDIAN=	be
MAKEDEVTARGETS=	floppy
IMAGEDEPENDS+=	${ARCHDIR}/disktab.shadow ${ARCHDIR}/dot.profile \
		${ARCHDIR}/install.md ${ARCHDIR}/termcap.vt

HACKSRC= ${DISTRIBDIR}/utils/libhack
.include "${HACKSRC}/Makefile.inc"

${CRUNCHBIN}: libhack.o
