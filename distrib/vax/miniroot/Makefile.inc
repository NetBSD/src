# $NetBSD: Makefile.inc,v 1.7 2002/03/29 06:37:53 matt Exp $

MAKEFS_FLAGS=	-o density=5k
IMAGESIZE=	7m
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

image_md_post:
.if ${USETOOLS} == "yes"
	${TOOLDIR}/bin/nbvax-installboot ${IMAGE} ${DESTDIR}/usr/mdec/xxboot
.else
	${DESTDIR}/usr/mdec/installboot ${IMAGE} ${DESTDIR}/usr/mdec/xxboot
.endif
