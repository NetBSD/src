#	$NetBSD: Makefile.inc,v 1.2 2009/04/07 11:49:17 joerg Exp $

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

.include "${DISTRIBDIR}/common/Makefile.dhcpcd"
