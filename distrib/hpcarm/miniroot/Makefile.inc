#	$NetBSD: Makefile.inc,v 1.1 2005/12/30 11:47:19 rjs Exp $

DBG=		-Os

IMAGESIZE=	4352k
MAKEFS_FLAGS=	-o density=2k
IMAGEENDIAN=	le
MAKEDEVTARGETS=	all
LISTS+=		${DISTRIBDIR}/common/list.sysinst
MTREECONF+=	${.CURDIR}/mtree.usr.install
IMAGEDEPENDS+=	${ARCHDIR}/dot.profile ${ARCHDIR}/termcap.vt100 \
		${DESTDIR}/.profile ${DESTDIR}/etc/disktab
SMALLPROG_INET6=1
