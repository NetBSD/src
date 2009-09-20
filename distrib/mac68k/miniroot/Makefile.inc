#	$NetBSD: Makefile.inc,v 1.6 2009/09/20 23:19:54 abs Exp $

IMAGESIZE=	4m
IMAGEENDIAN=	be
MAKEFS_FLAGS=	-o density=12k
MAKEDEVTARGETS=	raminst
IMAGEDEPENDS+=	${ARCHDIR}/dot.profile ${ARCHDIR}/install.md \
		${DISTRIBDIR}/common/termcap
