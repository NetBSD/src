#	$NetBSD: Makefile.inc,v 1.13 2003/02/01 05:03:42 tsutsui Exp $

DBG=		-Os

IMAGESIZE=	4m
MAKEFS_FLAGS=	-o density=2k
IMAGEENDIAN=	le
MAKEDEVTARGETS=	all
LISTS+=		${DISTRIBDIR}/common/list.sysinst
MTREECONF+=	${.CURDIR}/mtree.usr.install
IMAGEDEPENDS+=	${ARCHDIR}/dot.profile ${ARCHDIR}/termcap.vt100 \
		${DESTDIR}/.profile ${DESTDIR}/etc/disktab
SMALLPROG_INET6=1
