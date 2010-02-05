#	$NetBSD: Makefile.inc,v 1.11 2010/02/05 10:01:19 roy Exp $

IMAGESIZE=	7m
MAKEFS_FLAGS=	-o density=4096
IMAGEENDIAN=	be
MAKEDEVTARGETS=	all
IMAGEDEPENDS+=	${ARCHDIR}/dot.profile ${ARCHDIR}/install.md \
		${KERNOBJDIR}/GENERIC/netbsd \
		${DESTDIR}/rescue/terminfo.db
LISTS+=		${DISTRIBDIR}/common/list.sysinst
