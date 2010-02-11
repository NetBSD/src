#	$NetBSD: Makefile.inc,v 1.5 2010/02/11 09:06:48 roy Exp $

DBG=		-O

IMAGESIZE=	4m
MAKEFS_FLAGS=	-o density=2k
IMAGEENDIAN=	le
MAKEDEVTARGETS=	all
#LISTS+=		${DISTRIBDIR}/common/list.sysinst
MTREECONF+=	${.CURDIR}/mtree.usr.install
IMAGEDEPENDS+=	${ARCHDIR}/dot.profile \
		${DESTDIR}/.profile ${DESTDIR}/etc/spwd.db
