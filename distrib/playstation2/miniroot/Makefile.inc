#	$NetBSD: Makefile.inc,v 1.3 2002/03/06 12:04:53 lukem Exp $

IMAGESIZE=	5m
IMAGEENDIAN=	be
MAKEDEVTARGETS=	all
LISTS+=		${DISTRIBDIR}/common/list.sysinst
MTREECONF+=	${.CURDIR}/mtree.usr.install
IMAGEDEPENDS+=	${ARCHDIR}/dot.profile ${ARCHDIR}/termcap.vt100 \
		${DESTDIR}/.profile ${DESTDIR}/etc/disktab \
		${DESTDIR}/etc/spwd.db
