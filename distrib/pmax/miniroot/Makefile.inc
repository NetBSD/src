#	$NetBSD: Makefile.inc,v 1.6 2002/03/23 03:22:25 shin Exp $

IMAGESIZE=	32m
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
