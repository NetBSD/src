#	$NetBSD: Makefile.inc,v 1.12.14.1 2012/11/25 10:10:03 jdc Exp $

IMAGESIZE=	7m
MAKEFS_FLAGS=	-o density=4096
IMAGEENDIAN=	be
MAKEDEVTARGETS=	all ipty
IMAGEDEPENDS+=	${ARCHDIR}/dot.profile ${ARCHDIR}/install.md \
		${KERNOBJDIR}/GENERIC/netbsd
LISTS+=		${DISTRIBDIR}/common/list.sysinst
