#	$NetBSD: Makefile.inc,v 1.6 2010/05/03 14:14:35 tsutsui Exp $

WARNS=		1
DBG=		-Os

IMAGESIZE=	4m
MAKEFS_FLAGS=	-o density=2k
IMAGEENDIAN=	le
MAKEDEVTARGETS=	all
LISTS+=		${DISTRIBDIR}/common/list.sysinst.en
MTREECONF+=	${.CURDIR}/mtree.usr.install
IMAGEDEPENDS+=	${ARCHDIR}/dot.profile
