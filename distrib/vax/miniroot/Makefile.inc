# $NetBSD: Makefile.inc,v 1.10 2002/04/12 03:21:12 lukem Exp $

IMAGESIZE=	7m
MAKEFS_FLAGS=	-o density=5k
IMAGEENDIAN=	le
MAKEDEVTARGETS=	all
LISTS+=		${DISTRIBDIR}/common/list.sysinst
IMAGEDEPENDS+=	${ARCHDIR}/../install.md \
		${ARCHDIR}/dot.profile \
		${DESTDIR}/etc/disktab \
		${DESTDIR}/etc/spwd.db \
		${DESTDIR}/usr/mdec/boot \
		${KERNOBJDIR}/GENERIC/netbsd \
		${_SRC_TOP_}/share/termcap/termcap.src

IMAGE_MD_POST=	${INSTALLBOOT} -m ${MACHINE} ${IMAGE} ${DESTDIR}/usr/mdec/xxboot
