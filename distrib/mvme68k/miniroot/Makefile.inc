#	$NetBSD: Makefile.inc,v 1.5 2002/04/03 19:23:08 scw Exp $

IMAGESIZE=	7m
MAKEFS_FLAGS=	-o density=4096
IMAGEENDIAN=	be
MAKEDEVTARGETS=	all
IMAGEDEPENDS+=	${ARCHDIR}/dot.profile ${ARCHDIR}/install.md \
		${DESTDIR}/etc/disktab ${DESTDIR}/etc/spwd.db \
		${KERNOBJDIR}/GENERIC/netbsd \
		${_SRC_TOP_}/share/termcap/termcap.src
LISTS+=		${DISTRIBDIR}/common/list.sysinst

image_md_post:

