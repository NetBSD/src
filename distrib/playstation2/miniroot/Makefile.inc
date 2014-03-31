#	$NetBSD: Makefile.inc,v 1.14 2014/03/31 11:25:48 martin Exp $

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
