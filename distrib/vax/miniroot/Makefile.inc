# $NetBSD: Makefile.inc,v 1.5 2002/03/29 00:15:07 matt Exp $

IMAGESIZE=	7m
IMAGEENDIAN=	le
MAKEDEVTARGETS=	all
LISTS+=		${DISTRIBDIR}/common/list.sysinst
IMAGEDEPENDS+=	${ARCHDIR}/../install.md ${ARCHDIR}/dot.profile \
		${DESTDIR}/etc/disktab ${DESTDIR}/etc/spwd.db \
		${DESTDIR}/usr/mdec/boot \
		${KERNOBJDIR}/GENERIC/netbsd \
		${_SRC_TOP_}/share/termcap/termcap.src

# XXXIMAGE installboot needs to be MI
image_md_post:
	${DESTDIR}/usr/mdec/installboot ${IMAGE} ${DESTDIR}/usr/mdec/xxboot
