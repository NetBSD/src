#	$NetBSD: Makefile.inc,v 1.8.30.1 2017/03/20 06:52:06 pgoyette Exp $

IMAGESIZE=	4m
IMAGEENDIAN=	be
MAKEFS_FLAGS+=	-o density=12k
MAKEDEVTARGETS=	raminst
IMAGEDEPENDS+=	${ARCHDIR}/dot.profile ${ARCHDIR}/install.md
