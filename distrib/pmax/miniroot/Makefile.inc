#	$NetBSD: Makefile.inc,v 1.14 2003/02/18 08:11:21 shin Exp $

DBG=		-Os -Wa,-xgot # XXX -xgot workaround for objcopy-crunchide

IMAGESIZE=	32m
MAKEFS_FLAGS=	-f 15
IMAGEENDIAN=	le
MAKEDEVTARGETS=	all
LISTS+=		${DISTRIBDIR}/common/list.sysinst
MTREECONF+=	${.CURDIR}/mtree.usr.install
IMAGEDEPENDS+=	${ARCHDIR}/dot.profile ${ARCHDIR}/termcap.rcons \
		${DESTDIR}/.profile ${DESTDIR}/boot.pmax \
		${DESTDIR}/etc/disktab \
		${KERNOBJDIR}/GENERIC/netbsd.aout \
		${KERNOBJDIR}/GENERIC/netbsd.ecoff \
		${KERNOBJDIR}/INSTALL/nfsnetbsd.ecoff

IMAGEPOSTBUILD=	${INSTALLBOOT} -v -m ${MACHINE} ${IMAGE} ${DESTDIR}/usr/mdec/bootxx_ffs
