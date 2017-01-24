#	$NetBSD: Makefile.inc,v 1.9 2017/01/24 18:04:03 christos Exp $

IMAGESIZE=	4m
IMAGEENDIAN=	be
MAKEFS_FLAGS+=	-o density=12k
MAKEDEVTARGETS=	raminst
IMAGEDEPENDS+=	${ARCHDIR}/dot.profile ${ARCHDIR}/install.md
