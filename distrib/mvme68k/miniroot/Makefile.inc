#	$NetBSD: Makefile.inc,v 1.7 2002/04/26 15:50:39 lukem Exp $

IMAGESIZE=	7m
MAKEFS_FLAGS=	-o density=4096
IMAGEENDIAN=	be
MAKEDEVTARGETS=	all
IMAGEDEPENDS+=	${ARCHDIR}/dot.profile ${ARCHDIR}/install.md \
		${DESTDIR}/etc/disktab ${DESTDIR}/etc/spwd.db \
		${KERNOBJDIR}/GENERIC/netbsd \
		${NETBSDSRCDIR}/share/termcap/termcap.src
LISTS+=		${DISTRIBDIR}/common/list.sysinst
