#	$NetBSD: Makefile.inc,v 1.8 2002/03/23 08:24:37 shin Exp $

IMAGESIZE=	4m
IMAGEENDIAN=	le
MAKEDEVTARGETS=	all
LISTS+=		${DISTRIBDIR}/common/list.sysinst
MTREECONF+=	${.CURDIR}/mtree.usr.install
IMAGEDEPENDS+=	${ARCHDIR}/dot.profile ${ARCHDIR}/termcap.vt100 \
		${DESTDIR}/.profile ${DESTDIR}/etc/disktab \
		${DESTDIR}/etc/spwd.db
MAKEFS_FLAGS=	-o density=2048
