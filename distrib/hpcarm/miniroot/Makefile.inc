#	$NetBSD: Makefile.inc,v 1.5 2010/02/05 10:01:18 roy Exp $

DBG=		-Os

IMAGESIZE=	4352k
MAKEFS_FLAGS=	-o density=2k
IMAGEENDIAN=	le
MAKEDEVTARGETS=	all
LISTS+=		${DISTRIBDIR}/common/list.sysinst
MTREECONF+=	${.CURDIR}/mtree.usr.install
IMAGEDEPENDS+=	${ARCHDIR}/dot.profile \
		${DESTDIR}/rescue/terminfo.db \
		${DESTDIR}/.profile
SMALLPROG_INET6=1

.include "${DISTRIBDIR}/common/Makefile.dhcpcd"
