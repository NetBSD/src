#	$NetBSD: Makefile.inc,v 1.8 2002/04/12 03:21:09 lukem Exp $

IMAGESIZE=	5m
IMAGEENDIAN=	be
MAKEDEVTARGETS=	floppy
IMAGEDEPENDS+=	${ARCHDIR}/disktab.shadow ${ARCHDIR}/dot.profile \
		${ARCHDIR}/install.md ${ARCHDIR}/termcap.vt \
		${DESTDIR}/boot.amiga ${DESTDIR}/etc/spwd.db \
		${KERNOBJDIR}/INSTALL/netbsd

IMAGE_MD_POST=	dd if=${DESTDIR}/usr/mdec/bootxx_ffs of=${IMAGE} bs=8192 conv=notrunc
