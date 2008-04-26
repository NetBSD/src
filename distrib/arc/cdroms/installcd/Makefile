#	$NetBSD: Makefile,v 1.2 2008/04/26 10:45:49 tsutsui Exp $
CDBASE=		arccd			# gives ${CDBASE}.iso
CDRELEASE=	true			# include $RELEASEDIR/$MACHINE
CDKERNELS=	netbsd-RAMDISK.gz netbsd
CDINSTKERNEL=	../../instkernel
BOOT=		${DESTDIR}/usr/mdec/boot

image_md_post:
	${CP} ${BOOT} cdrom

.include "${.CURDIR}/../../../common/Makefile.bootcd"
