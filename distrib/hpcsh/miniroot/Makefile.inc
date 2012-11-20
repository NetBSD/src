#	$NetBSD: Makefile.inc,v 1.7.12.1 2012/11/20 02:57:42 tls Exp $

WARNS=		1
DBG=		-Os

IMAGESIZE=	4m
MAKEFS_FLAGS=	-o density=2k
IMAGEENDIAN=	le
MAKEDEVTARGETS=	all ipty
LISTS+=		${DISTRIBDIR}/common/list.sysinst.en
MTREECONF+=	${.CURDIR}/mtree.usr.install
IMAGEDEPENDS+=	${ARCHDIR}/dot.profile

.include "${DISTRIBDIR}/common/Makefile.dhcpcd"
