#	$NetBSD: Makefile.inc,v 1.21 2021/02/25 21:46:55 rin Exp $

IMAGESIZE=	7m
MAKEFS_FLAGS+=   -o density=4k
IMAGEENDIAN=	be
MAKEDEVTARGETS=	floppy
IMAGEDEPENDS+=	${ARCHDIR}/dot.profile \
		${ARCHDIR}/install.md \
		${DESTDIR}/usr/mdec/boot.amiga \
		${KERNOBJDIR}/INSTALL/netbsd

IMAGEPOSTBUILD=	dd if=${DESTDIR}/usr/mdec/bootxx_ffsv1 of=${IMAGE} bs=8192 conv=notrunc
