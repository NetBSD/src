#	$NetBSD: Makefile.inc,v 1.8 2002/04/26 15:50:31 lukem Exp $

IMAGESIZE=	7m
IMAGEENDIAN=	be
MAKEDEVTARGETS=	all
MAKEFS_FLAGS=	-o density=4k
IMAGEDEPENDS+=	${ARCHDIR}/dot.profile ${ARCHDIR}/install.md \
		${DESTDIR}/etc/disktab \
		${KERNOBJDIR}/INSTALL/netbsd \
		${NETBSDSRCDIR}/share/termcap/termcap.src
