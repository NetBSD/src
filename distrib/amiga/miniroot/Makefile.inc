#	$NetBSD: Makefile.inc,v 1.7 2002/03/06 12:04:50 lukem Exp $

IMAGESIZE=	5m
IMAGEENDIAN=	be
MAKEDEVTARGETS=	floppy
IMAGEDEPENDS+=	${ARCHDIR}/disktab.shadow ${ARCHDIR}/dot.profile \
		${ARCHDIR}/install.md ${ARCHDIR}/termcap.vt \
		${DESTDIR}/boot.amiga ${DESTDIR}/etc/spwd.db \
		${KERNOBJDIR}/INSTALL/netbsd

image_md_post:
	dd if=${DESTDIR}/usr/mdec/bootxx_ffs of=${IMAGE} bs=8192 conv=notrunc
