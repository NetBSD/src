#	$NetBSD: Makefile.inc,v 1.7 2002/03/23 05:15:20 shin Exp $

IMAGESIZE=	4m
IMAGEENDIAN=	le
MAKEDEVTARGETS=	all
LISTS+=		${DISTRIBDIR}/common/list.sysinst
MTREECONF+=	${.CURDIR}/mtree.usr.install
IMAGEDEPENDS+=	${ARCHDIR}/dot.profile ${ARCHDIR}/termcap.vt100 \
		${DESTDIR}/.profile ${DESTDIR}/etc/disktab \
		${DESTDIR}/etc/spwd.db
