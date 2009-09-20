#	$NetBSD: Makefile.inc,v 1.16 2009/09/20 22:46:12 abs Exp $

DBG=		-Os

IMAGESIZE=	4352k
MAKEFS_FLAGS=	-o density=2k
IMAGEENDIAN=	le
MAKEDEVTARGETS=	all
LISTS+=		${DISTRIBDIR}/common/list.sysinst
MTREECONF+=	${.CURDIR}/mtree.usr.install
IMAGEDEPENDS+=	${ARCHDIR}/dot.profile ${ARCHDIR}/termcap.vt100 \
		${DESTDIR}/.profile
SMALLPROG_INET6=1

.include "${DISTRIBDIR}/common/Makefile.dhcpcd"
