#	$NetBSD: Makefile.inc,v 1.10 2009/09/20 23:19:52 abs Exp $

IMAGESIZE=	7m
IMAGEENDIAN=	be
MAKEDEVTARGETS=	all
MAKEFS_FLAGS=	-o density=4k
IMAGEDEPENDS+=	${ARCHDIR}/dot.profile ${ARCHDIR}/install.md \
		${KERNOBJDIR}/INSTALL/netbsd \
		${DISTRIBDIR}/common/termcap
