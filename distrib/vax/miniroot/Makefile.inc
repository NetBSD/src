# $NetBSD: Makefile.inc,v 1.8 2002/04/01 12:16:14 lukem Exp $

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

image_md_post:
.if ${USETOOLS} == "yes"
	${TOOLDIR}/bin/nbvax-installboot ${IMAGE} ${DESTDIR}/usr/mdec/xxboot
.else
	${DESTDIR}/usr/mdec/installboot ${IMAGE} ${DESTDIR}/usr/mdec/xxboot
.endif
