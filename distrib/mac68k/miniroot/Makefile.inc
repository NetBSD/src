#	$NetBSD: Makefile.inc,v 1.5 2003/05/02 22:40:19 he Exp $

IMAGESIZE=	4m
IMAGEENDIAN=	be
MAKEFS_FLAGS=	-o density=12k
MAKEDEVTARGETS=	raminst
IMAGEDEPENDS+=	${ARCHDIR}/dot.profile ${ARCHDIR}/install.md \
		${ARCHDIR}/termcap
