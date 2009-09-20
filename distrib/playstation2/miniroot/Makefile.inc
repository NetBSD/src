#	$NetBSD: Makefile.inc,v 1.11 2009/09/20 22:46:12 abs Exp $

DBG=		-Os

IMAGESIZE=	5m
MAKEFS_FLAGS=	-o density=2k
IMAGEENDIAN=	le
MAKEDEVTARGETS=	all
LISTS+=		${DISTRIBDIR}/common/list.sysinst
MTREECONF+=	${.CURDIR}/mtree.usr.install
IMAGEDEPENDS+=	${ARCHDIR}/dot.profile ${ARCHDIR}/termcap.vt100 \
		${DESTDIR}/.profile
SMALLPROG_INET6=1
