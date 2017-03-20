#	$NetBSD: Makefile.inc,v 1.14.10.1 2017/03/20 06:52:07 pgoyette Exp $

DBG=		-Os

IMAGESIZE=	5m
MAKEFS_FLAGS+=	-o density=2k
IMAGEENDIAN=	le
MAKEDEVTARGETS=	all
LISTS+=		${DISTRIBDIR}/common/list.sysinst
MTREECONF+=	${.CURDIR}/mtree.usr.install
IMAGEDEPENDS+=	${ARCHDIR}/dot.profile ${DISTRIBDIR}/common/termcap \
		${DESTDIR}/.profile
SMALLPROG_INET6=1
