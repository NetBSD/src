#	$NetBSD: Makefile.inc,v 1.10.2.2 2002/05/29 04:58:04 lukem Exp $

IMAGESIZE=	5m
MAKEFS_FLAGS=   -o density=8k
IMAGEENDIAN=	be
MAKEDEVTARGETS=	floppy
IMAGEDEPENDS+=	${ARCHDIR}/disktab.shadow ${ARCHDIR}/dot.profile \
		${ARCHDIR}/install.md ${ARCHDIR}/termcap.vt \
		${DESTDIR}/usr/mdec/boot.amiga \
		${KERNOBJDIR}/INSTALL/netbsd

IMAGEPOSTBUILD=	dd if=${DESTDIR}/usr/mdec/bootxx_ffs of=${IMAGE} bs=8192 conv=notrunc
