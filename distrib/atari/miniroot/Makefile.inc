#	$NetBSD: Makefile.inc,v 1.7 2002/03/25 07:43:39 lukem Exp $

IMAGESIZE=	1080k
IMAGEENDIAN=	be
MAKEDEVTARGETS=	floppy
IMAGEDEPENDS+=	${ARCHDIR}/disktab.shadow ${ARCHDIR}/dot.profile \
		${ARCHDIR}/install.md ${ARCHDIR}/termcap.vt

HACKSRC= ${DISTRIBDIR}/utils/libhack
.include "${HACKSRC}/Makefile.inc"
HACKOBJS+=	gethost.o

${CRUNCHBIN}: libhack.o
