#	$NetBSD: Makefile.inc,v 1.14 2017/01/24 18:04:02 christos Exp $

IMAGESIZE=	7m
IMAGEENDIAN=	be
MAKEDEVTARGETS=	all ipty
MAKEFS_FLAGS+=	-o density=4k
IMAGEDEPENDS+=	${ARCHDIR}/dot.profile ${ARCHDIR}/install.md \
		${KERNOBJDIR}/INSTALL/netbsd
