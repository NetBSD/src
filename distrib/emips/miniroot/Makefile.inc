#	$NetBSD: Makefile.inc,v 1.5 2024/05/24 13:26:35 tsutsui Exp $

DBG=		-Os

IMAGESIZE=	32m
MAKEFS_FLAGS+=	-f 15
IMAGEENDIAN=	be
MAKEDEVTARGETS=	all ipty
LISTS+=		${DISTRIBDIR}/common/list.sysinst
MTREECONF+=	${.CURDIR}/mtree.usr.install
IMAGEDEPENDS+=	${ARCHDIR}/dot.profile \
		${DESTDIR}/.profile ${DESTDIR}/boot.emips \
		${DESTDIR}/etc/disktab \
		${KERNOBJDIR}/GENERIC/netbsd
