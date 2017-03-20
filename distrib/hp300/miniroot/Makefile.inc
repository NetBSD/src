#	$NetBSD: Makefile.inc,v 1.13.14.1 2017/03/20 06:52:05 pgoyette Exp $

IMAGESIZE=	7m
IMAGEENDIAN=	be
MAKEDEVTARGETS=	all ipty
MAKEFS_FLAGS+=	-o density=4k
IMAGEDEPENDS+=	${ARCHDIR}/dot.profile ${ARCHDIR}/install.md \
		${KERNOBJDIR}/INSTALL/netbsd
