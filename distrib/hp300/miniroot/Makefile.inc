#	$NetBSD: Makefile.inc,v 1.5 2002/03/14 04:51:20 gmcgarry Exp $

IMAGESIZE=	7m
IMAGEENDIAN=	be
MAKEDEVTARGETS=	all
IMAGEDEPENDS+=	${ARCHDIR}/dot.profile ${ARCHDIR}/install.md \
		${DESTDIR}/etc/disktab \
		${KERNOBJDIR}/INSTALL/netbsd \
		${_SRC_TOP_}/share/termcap/termcap.src

image_md_post:
