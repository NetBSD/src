#	$NetBSD: Makefile.inc,v 1.9 2009/09/20 22:46:11 abs Exp $

IMAGESIZE=	7m
IMAGEENDIAN=	be
MAKEDEVTARGETS=	all
MAKEFS_FLAGS=	-o density=4k
IMAGEDEPENDS+=	${ARCHDIR}/dot.profile ${ARCHDIR}/install.md \
		${KERNOBJDIR}/INSTALL/netbsd \
		${NETBSDSRCDIR}/share/termcap/termcap.src
