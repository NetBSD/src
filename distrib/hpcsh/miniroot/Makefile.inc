#	$NetBSD: Makefile.inc,v 1.9 2017/01/24 18:04:02 christos Exp $

WARNS=		1
DBG=		-Os

IMAGESIZE=	4m
MAKEFS_FLAGS+=	-o density=2k
IMAGEENDIAN=	le
MAKEDEVTARGETS=	all ipty
LISTS+=		${DISTRIBDIR}/common/list.sysinst.en
MTREECONF+=	${.CURDIR}/mtree.usr.install
IMAGEDEPENDS+=	${ARCHDIR}/dot.profile

.include "${DISTRIBDIR}/common/Makefile.dhcpcd"
