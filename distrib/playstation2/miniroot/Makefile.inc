#	$NetBSD: Makefile.inc,v 1.7.2.1 2002/05/29 04:59:10 lukem Exp $

DBG=		-Os -Wa,-xgot # XXX -xgot workaround for objcopy-crunchide

IMAGESIZE=	5m
MAKEFS_FLAGS=	-o density=2k
IMAGEENDIAN=	le
MAKEDEVTARGETS=	all
LISTS+=		${DISTRIBDIR}/common/list.sysinst
MTREECONF+=	${.CURDIR}/mtree.usr.install
IMAGEDEPENDS+=	${ARCHDIR}/dot.profile ${ARCHDIR}/termcap.vt100 \
		${DESTDIR}/.profile ${DESTDIR}/etc/disktab
