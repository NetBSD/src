#	$NetBSD: Makefile.inc,v 1.3 2002/03/25 21:12:51 scw Exp $

IMAGESIZE=	7m
IMAGEENDIAN=	be
MAKEDEVTARGETS=	all
IMAGEDEPENDS+=	${ARCHDIR}/dot.profile ${ARCHDIR}/install.md \
		${DESTDIR}/etc/disktab ${DESTDIR}/etc/spwd.db \
		${KERNOBJDIR}/GENERIC/netbsd \
		${_SRC_TOP_}/share/termcap/termcap.src
MAKEFS_FLAGS=	-f 94
LISTS+=		${DISTRIBDIR}/common/list.sysinst

image_md_post:

