#	$NetBSD: Makefile.inc,v 1.4 2002/03/23 03:22:56 shin Exp $

IMAGESIZE=	5m
IMAGEENDIAN=	le
MAKEDEVTARGETS=	all
LISTS+=		${DISTRIBDIR}/common/list.sysinst
MTREECONF+=	${.CURDIR}/mtree.usr.install
IMAGEDEPENDS+=	${ARCHDIR}/dot.profile ${ARCHDIR}/termcap.vt100 \
		${DESTDIR}/.profile ${DESTDIR}/etc/disktab \
		${DESTDIR}/etc/spwd.db
