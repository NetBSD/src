#	$NetBSD: Makefile.inc,v 1.7 2002/04/01 12:16:13 lukem Exp $

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

# XXXIMAGE: installboot needs to be MI and work on image files
image_md_post:
	${DESTDIR}/usr/mdec/installboot ${IMAGE} ${DESTDIR}/usr/mdec/bootxx_ffs
