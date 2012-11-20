#	$NetBSD: Makefile.inc,v 1.2.8.1 2012/11/20 02:57:41 tls Exp $

DBG=		-Os

IMAGESIZE=	32m
MAKEFS_FLAGS=	-f 15
IMAGEENDIAN=	be
MAKEDEVTARGETS=	all ipty
LISTS+=		${DISTRIBDIR}/common/list.sysinst
MTREECONF+=	${.CURDIR}/mtree.usr.install
IMAGEDEPENDS+=	${ARCHDIR}/dot.profile \
		${DESTDIR}/.profile ${DESTDIR}/boot.emips \
		${DESTDIR}/etc/disktab \
		${KERNOBJDIR}/GENERIC/netbsd

IMAGEPOSTBUILD=	${TOOL_INSTALLBOOT} -v -m ${MACHINE} ${IMAGE} ${DESTDIR}/usr/mdec/bootxx_ffs
