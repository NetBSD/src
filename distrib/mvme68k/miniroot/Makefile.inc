#	$NetBSD: Makefile.inc,v 1.1 2002/03/06 12:04:52 lukem Exp $

IMAGESIZE=	7m
IMAGEENDIAN=	be
MAKEDEVTARGETS=	all
IMAGEDEPENDS+=	${ARCHDIR}/dot.profile ${ARCHDIR}/install.md \
		${DESTDIR}/etc/disktab ${DESTDIR}/etc/spwd.db \
		${KERNOBJDIR}/GENERIC/netbsd \
		${_SRC_TOP_}/share/termcap/termcap.src
