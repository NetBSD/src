#	$NetBSD: Makefile.inc,v 1.5 2002/03/27 13:32:08 uch Exp $

IMAGESIZE=	5m
IMAGEENDIAN=	le
MAKEDEVTARGETS=	all
LISTS+=		${DISTRIBDIR}/common/list.sysinst
MTREECONF+=	${.CURDIR}/mtree.usr.install
IMAGEDEPENDS+=	${ARCHDIR}/dot.profile ${ARCHDIR}/termcap.vt100 \
		${DESTDIR}/.profile ${DESTDIR}/etc/disktab \
		${DESTDIR}/etc/spwd.db
MAKEFS_FLAGS=	-o density=2048
