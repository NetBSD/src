#	$NetBSD: Makefile.inc,v 1.9 2002/04/16 20:46:19 heinz Exp $

IMAGESIZE=	5m
IMAGEENDIAN=	be
MAKEDEVTARGETS=	floppy
IMAGEDEPENDS+=	${ARCHDIR}/disktab.shadow ${ARCHDIR}/dot.profile \
		${ARCHDIR}/install.md ${ARCHDIR}/termcap.vt \
		${DESTDIR}/usr/mdec/boot.amiga ${DESTDIR}/etc/spwd.db \
		${KERNOBJDIR}/INSTALL/netbsd

IMAGE_MD_POST=	dd if=${DESTDIR}/usr/mdec/bootxx_ffs of=${IMAGE} bs=8192 conv=notrunc
