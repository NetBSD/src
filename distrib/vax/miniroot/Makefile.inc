# $NetBSD: Makefile.inc,v 1.4 2002/03/06 12:04:55 lukem Exp $

IMAGESIZE=	7m
IMAGEENDIAN=	le
MAKEDEVTARGETS=	all
LISTS+=		${DISTRIBDIR}/common/list.sysinst
IMAGEDEPENDS+=	${ARCHDIR}/../install.md ${ARCHDIR}/dot.profile \
		${DESTDIR}/etc/disktab ${DESTDIR}/etc/spwd.db \
		${DESTDIR}/usr/mdec/boot \
		${KERNOBJDIR}/GENERIC/netbsd \
		${_SRC_TOP_}/share/termcap/termcap.src

# XXXIMAGE: disklabel needs to be MI and work on image files
image_md_post:
	${DESTDIR}/sbin/disklabel -B -b ${DESTDIR}/usr/mdec/xxboot ${IMAGE}
