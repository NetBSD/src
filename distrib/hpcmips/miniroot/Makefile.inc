#	$NetBSD: Makefile.inc,v 1.19.6.1 2012/10/30 18:48:31 yamt Exp $

DBG=		-Os

IMAGESIZE=	4352k
MAKEFS_FLAGS=	-o density=2k
IMAGEENDIAN=	le
MAKEDEVTARGETS=	all ipty
LISTS+=		${DISTRIBDIR}/common/list.sysinst
MTREECONF+=	${.CURDIR}/mtree.usr.install
IMAGEDEPENDS+=	${ARCHDIR}/dot.profile \
		${DESTDIR}/.profile
SMALLPROG_INET6=1

.include "${DISTRIBDIR}/common/Makefile.dhcpcd"
