#	$NetBSD: Makefile.inc,v 1.8.34.1 2017/04/21 16:51:11 bouyer Exp $

IMAGESIZE=	4m
IMAGEENDIAN=	be
MAKEFS_FLAGS+=	-o density=12k
MAKEDEVTARGETS=	raminst
IMAGEDEPENDS+=	${ARCHDIR}/dot.profile ${ARCHDIR}/install.md
