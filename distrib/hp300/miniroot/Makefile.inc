#	$NetBSD: Makefile.inc,v 1.11 2010/02/05 10:01:18 roy Exp $

IMAGESIZE=	7m
IMAGEENDIAN=	be
MAKEDEVTARGETS=	all
MAKEFS_FLAGS=	-o density=4k
IMAGEDEPENDS+=	${ARCHDIR}/dot.profile ${ARCHDIR}/install.md \
		${KERNOBJDIR}/INSTALL/netbsd \
		${DESTDIR}/rescue/terminfo.db
