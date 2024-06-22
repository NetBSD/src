#	$NetBSD: Makefile,v 1.3.8.1 2024/06/22 10:57:11 martin Exp $
CDBASE=		hp300cd			# gives ${CDBASE}.iso
CDRELEASE=	true			# include $RELEASEDIR/$RELEASEMACHINEDIR
CDRELEASE_NODEBUG=	true

CDKERNELS=	netbsd-RAMDISK.gz netbsd
CDINSTKERNEL=	../../instkernel

SYS_UBOOT=	SYS_UBOOT
BOOTDIR=	${DESTDIR}/usr/mdec/rbootd

# make the CD bootable
prepare_md_post:
	${INSTALL} ${COPY} -m 0644 ${BOOTDIR}/${SYS_UBOOT} cdrom

image_md_post:
	${TOOL_INSTALLBOOT} -m ${MACHINE} \
	    ${CDIMAGE} ${BOOTDIR}/${SYS_UBOOT} /${SYS_UBOOT}

.include "${.CURDIR}/../../../common/Makefile.bootcd"
