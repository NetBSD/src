#	$NetBSD: Makefile.inc,v 1.12 2009/09/20 23:19:54 abs Exp $

DBG=		-Os

IMAGESIZE=	5m
MAKEFS_FLAGS=	-o density=2k
IMAGEENDIAN=	le
MAKEDEVTARGETS=	all
LISTS+=		${DISTRIBDIR}/common/list.sysinst
MTREECONF+=	${.CURDIR}/mtree.usr.install
IMAGEDEPENDS+=	${ARCHDIR}/dot.profile ${DISTRIBDIR}/common/termcap \
		${DESTDIR}/.profile
SMALLPROG_INET6=1
