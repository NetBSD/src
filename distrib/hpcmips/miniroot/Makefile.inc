#	$NetBSD: Makefile.inc,v 1.12 2002/10/17 02:05:36 lukem Exp $

DBG=		-Os -Wa,-xgot # XXX -xgot workaround for objcopy-crunchide

IMAGESIZE=	4m
MAKEFS_FLAGS=	-o density=2k
IMAGEENDIAN=	le
MAKEDEVTARGETS=	all
LISTS+=		${DISTRIBDIR}/common/list.sysinst
MTREECONF+=	${.CURDIR}/mtree.usr.install
IMAGEDEPENDS+=	${ARCHDIR}/dot.profile ${ARCHDIR}/termcap.vt100 \
		${DESTDIR}/.profile ${DESTDIR}/etc/disktab
SMALLPROG_INET6=1
