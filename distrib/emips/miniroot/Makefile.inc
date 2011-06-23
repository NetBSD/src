#	$NetBSD: Makefile.inc,v 1.1.4.1 2011/06/23 14:18:17 cherry Exp $

DBG=		-Os

IMAGESIZE=	32m
MAKEFS_FLAGS=	-f 15
IMAGEENDIAN=	be
MAKEDEVTARGETS=	all
LISTS+=		${DISTRIBDIR}/common/list.sysinst
MTREECONF+=	${.CURDIR}/mtree.usr.install
IMAGEDEPENDS+=	${ARCHDIR}/dot.profile \
		${DESTDIR}/.profile ${DESTDIR}/boot.emips \
		${DESTDIR}/etc/disktab \
		${KERNOBJDIR}/GENERIC/netbsd

IMAGEPOSTBUILD=	${TOOL_INSTALLBOOT} -v -m ${MACHINE} ${IMAGE} ${DESTDIR}/usr/mdec/bootxx_ffs
