#	$NetBSD: Makefile.inc,v 1.12.12.1 2012/11/20 02:57:41 tls Exp $

IMAGESIZE=	7m
IMAGEENDIAN=	be
MAKEDEVTARGETS=	all ipty
MAKEFS_FLAGS=	-o density=4k
IMAGEDEPENDS+=	${ARCHDIR}/dot.profile ${ARCHDIR}/install.md \
		${KERNOBJDIR}/INSTALL/netbsd
