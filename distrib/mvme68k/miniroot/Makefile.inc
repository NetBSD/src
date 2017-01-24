#	$NetBSD: Makefile.inc,v 1.14 2017/01/24 18:04:03 christos Exp $

IMAGESIZE=	7m
MAKEFS_FLAGS+=	-o density=4096
IMAGEENDIAN=	be
MAKEDEVTARGETS=	all ipty
IMAGEDEPENDS+=	${ARCHDIR}/dot.profile ${ARCHDIR}/install.md \
		${KERNOBJDIR}/GENERIC/netbsd
LISTS+=		${DISTRIBDIR}/common/list.sysinst
