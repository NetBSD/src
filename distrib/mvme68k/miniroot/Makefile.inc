#	$NetBSD: Makefile.inc,v 1.12.6.1 2012/10/30 18:48:34 yamt Exp $

IMAGESIZE=	7m
MAKEFS_FLAGS=	-o density=4096
IMAGEENDIAN=	be
MAKEDEVTARGETS=	all ipty
IMAGEDEPENDS+=	${ARCHDIR}/dot.profile ${ARCHDIR}/install.md \
		${KERNOBJDIR}/GENERIC/netbsd
LISTS+=		${DISTRIBDIR}/common/list.sysinst
