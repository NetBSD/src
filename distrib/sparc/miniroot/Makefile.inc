#	$NetBSD: Makefile.inc,v 1.8 2002/05/07 11:35:44 pk Exp $

IMAGESIZE=	7680k
MAKEFS_FLAGS=   -o density=4k
IMAGEENDIAN=	be
MAKEDEVTARGETS=	all
LISTS+=		${DISTRIBDIR}/common/list.sysinst
IMAGEDEPENDS+=	${ARCHDIR}/../install.md ${ARCHDIR}/dot.profile \
		${ARCHDIR}/termcap.src \
		${DESTDIR}/etc/disktab ${DESTDIR}/etc/spwd.db \
		${DESTDIR}/usr/mdec/boot ${DESTDIR}/usr/share/misc/magic \
		${KERNOBJDIR}/GENERIC/netbsd

IMAGEPOSTBUILD=	${INSTALLBOOT} -m ${MACHINE} -v ${IMAGE} \
		${DESTDIR}/usr/mdec/bootxx /boot
