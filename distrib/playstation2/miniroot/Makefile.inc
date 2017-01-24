#	$NetBSD: Makefile.inc,v 1.15 2017/01/24 18:04:04 christos Exp $

DBG=		-Os

IMAGESIZE=	5m
MAKEFS_FLAGS+=	-o density=2k
IMAGEENDIAN=	le
MAKEDEVTARGETS=	all
LISTS+=		${DISTRIBDIR}/common/list.sysinst
MTREECONF+=	${.CURDIR}/mtree.usr.install
IMAGEDEPENDS+=	${ARCHDIR}/dot.profile ${DISTRIBDIR}/common/termcap \
		${DESTDIR}/.profile
SMALLPROG_INET6=1
