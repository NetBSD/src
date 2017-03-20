#	$NetBSD: Makefile.inc,v 1.8.14.1 2017/03/20 06:52:05 pgoyette Exp $

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
