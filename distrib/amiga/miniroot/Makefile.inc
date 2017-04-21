#	$NetBSD: Makefile.inc,v 1.19.4.1 2017/04/21 16:51:08 bouyer Exp $

IMAGESIZE=	7m
MAKEFS_FLAGS+=   -o density=4k
IMAGEENDIAN=	be
MAKEDEVTARGETS=	floppy
IMAGEDEPENDS+=	${ARCHDIR}/dot.profile \
		${ARCHDIR}/install.md \
		${DESTDIR}/usr/mdec/boot.amiga \
		${KERNOBJDIR}/INSTALL/netbsd

IMAGEPOSTBUILD=	dd if=${DESTDIR}/usr/mdec/bootxx_ffs of=${IMAGE} bs=8192 conv=notrunc
