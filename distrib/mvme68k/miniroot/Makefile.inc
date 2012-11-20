#	$NetBSD: Makefile.inc,v 1.12.12.1 2012/11/20 02:57:42 tls Exp $

IMAGESIZE=	7m
MAKEFS_FLAGS=	-o density=4096
IMAGEENDIAN=	be
MAKEDEVTARGETS=	all ipty
IMAGEDEPENDS+=	${ARCHDIR}/dot.profile ${ARCHDIR}/install.md \
		${KERNOBJDIR}/GENERIC/netbsd
LISTS+=		${DISTRIBDIR}/common/list.sysinst
