#	$NetBSD: Makefile.inc,v 1.7 2002/04/12 03:21:10 lukem Exp $

IMAGESIZE=	7m
IMAGEENDIAN=	be
MAKEDEVTARGETS=	all
MAKEFS_FLAGS=	-o density=4k
IMAGEDEPENDS+=	${ARCHDIR}/dot.profile ${ARCHDIR}/install.md \
		${DESTDIR}/etc/disktab \
		${KERNOBJDIR}/INSTALL/netbsd \
		${_SRC_TOP_}/share/termcap/termcap.src
