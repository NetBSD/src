#	$NetBSD: Makefile.inc,v 1.7.2.1 2002/05/29 04:58:48 lukem Exp $

IMAGESIZE=	7m
MAKEFS_FLAGS=	-o density=4096
IMAGEENDIAN=	be
MAKEDEVTARGETS=	all
IMAGEDEPENDS+=	${ARCHDIR}/dot.profile ${ARCHDIR}/install.md \
		${DESTDIR}/etc/disktab \
		${KERNOBJDIR}/GENERIC/netbsd \
		${NETBSDSRCDIR}/share/termcap/termcap.src
LISTS+=		${DISTRIBDIR}/common/list.sysinst
