#	$NetBSD: Makefile.inc,v 1.8 2010/02/11 09:06:49 roy Exp $

IMAGESIZE=	4m
IMAGEENDIAN=	be
MAKEFS_FLAGS=	-o density=12k
MAKEDEVTARGETS=	raminst
IMAGEDEPENDS+=	${ARCHDIR}/dot.profile ${ARCHDIR}/install.md
