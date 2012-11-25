#	$NetBSD: Makefile.inc,v 1.12.14.1 2012/11/25 10:10:03 jdc Exp $

IMAGESIZE=	7m
IMAGEENDIAN=	be
MAKEDEVTARGETS=	all ipty
MAKEFS_FLAGS=	-o density=4k
IMAGEDEPENDS+=	${ARCHDIR}/dot.profile ${ARCHDIR}/install.md \
		${KERNOBJDIR}/INSTALL/netbsd
