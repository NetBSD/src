#	$NetBSD: Makefile.inc,v 1.10 2009/09/20 23:19:54 abs Exp $

IMAGESIZE=	7m
MAKEFS_FLAGS=	-o density=4096
IMAGEENDIAN=	be
MAKEDEVTARGETS=	all
IMAGEDEPENDS+=	${ARCHDIR}/dot.profile ${ARCHDIR}/install.md \
		${KERNOBJDIR}/GENERIC/netbsd \
		${DISTRIBDIR}/common/termcap
LISTS+=		${DISTRIBDIR}/common/list.sysinst
