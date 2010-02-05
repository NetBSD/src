#	$NetBSD: Makefile.inc,v 1.7 2010/02/05 10:01:19 roy Exp $

IMAGESIZE=	4m
IMAGEENDIAN=	be
MAKEFS_FLAGS=	-o density=12k
MAKEDEVTARGETS=	raminst
IMAGEDEPENDS+=	${ARCHDIR}/dot.profile ${ARCHDIR}/install.md \
		${DESTDIR}/rescue/terminfo.db