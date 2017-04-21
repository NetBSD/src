#	$NetBSD: Makefile.inc,v 1.13.18.1 2017/04/21 16:51:11 bouyer Exp $

IMAGESIZE=	7m
MAKEFS_FLAGS+=	-o density=4096
IMAGEENDIAN=	be
MAKEDEVTARGETS=	all ipty
IMAGEDEPENDS+=	${ARCHDIR}/dot.profile ${ARCHDIR}/install.md \
		${KERNOBJDIR}/GENERIC/netbsd
LISTS+=		${DISTRIBDIR}/common/list.sysinst
