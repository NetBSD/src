#	$NetBSD: Makefile.inc,v 1.4 2010/02/05 10:01:18 roy Exp $

DBG=		-O

IMAGESIZE=	4m
MAKEFS_FLAGS=	-o density=2k
IMAGEENDIAN=	le
MAKEDEVTARGETS=	all
#LISTS+=		${DISTRIBDIR}/common/list.sysinst
MTREECONF+=	${.CURDIR}/mtree.usr.install
IMAGEDEPENDS+=	${ARCHDIR}/dot.profile \
		${DESTDIR}/rescue/terminfo.db \
		${DESTDIR}/.profile ${DESTDIR}/etc/spwd.db
