#	$NetBSD: Makefile.inc,v 1.17 2010/02/05 10:01:17 roy Exp $

IMAGESIZE=	6m
MAKEFS_FLAGS=   -o density=4k
IMAGEENDIAN=	be
MAKEDEVTARGETS=	floppy
IMAGEDEPENDS+=	${ARCHDIR}/dot.profile \
		${ARCHDIR}/install.md \
		${DESTDIR}/rescue/terminfo.db \
		${DESTDIR}/usr/mdec/boot.amiga \
		${KERNOBJDIR}/INSTALL/netbsd

IMAGEPOSTBUILD=	dd if=${DESTDIR}/usr/mdec/bootxx_ffs of=${IMAGE} bs=8192 conv=notrunc
