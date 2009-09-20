#	$NetBSD: Makefile.inc,v 1.15 2009/09/20 22:46:11 abs Exp $

IMAGESIZE=	6m
MAKEFS_FLAGS=   -o density=4k
IMAGEENDIAN=	be
MAKEDEVTARGETS=	floppy
IMAGEDEPENDS+=	${ARCHDIR}/dot.profile \
		${ARCHDIR}/install.md ${ARCHDIR}/termcap.vt \
		${DESTDIR}/usr/mdec/boot.amiga \
		${KERNOBJDIR}/INSTALL/netbsd

IMAGEPOSTBUILD=	dd if=${DESTDIR}/usr/mdec/bootxx_ffs of=${IMAGE} bs=8192 conv=notrunc
