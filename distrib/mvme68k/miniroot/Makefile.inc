#	$NetBSD: Makefile.inc,v 1.4 2002/04/01 12:16:12 lukem Exp $

IMAGESIZE=	7m
MAKEFS_FLAGS=	-f 94
IMAGEENDIAN=	be
MAKEDEVTARGETS=	all
IMAGEDEPENDS+=	${ARCHDIR}/dot.profile ${ARCHDIR}/install.md \
		${DESTDIR}/etc/disktab ${DESTDIR}/etc/spwd.db \
		${KERNOBJDIR}/GENERIC/netbsd \
		${_SRC_TOP_}/share/termcap/termcap.src
LISTS+=		${DISTRIBDIR}/common/list.sysinst

image_md_post:

