#	$NetBSD: Makefile.inc,v 1.6 2002/03/14 23:17:00 gmcgarry Exp $

IMAGESIZE=	7m
IMAGEENDIAN=	be
MAKEDEVTARGETS=	all
MAKEFS_FLAGS=	-o density=4k
IMAGEDEPENDS+=	${ARCHDIR}/dot.profile ${ARCHDIR}/install.md \
		${DESTDIR}/etc/disktab \
		${KERNOBJDIR}/INSTALL/netbsd \
		${_SRC_TOP_}/share/termcap/termcap.src

image_md_post:
