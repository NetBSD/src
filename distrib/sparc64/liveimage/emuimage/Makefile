#	$NetBSD: Makefile,v 1.1.4.2 2012/04/17 00:02:48 yamt Exp $

LIVEIMGBASE=	sparc64emu		# gives ${IMGBASE}.img

.include <bsd.own.mk>

KERN_SET=		kern-GENERIC
BOOTDISK=		sd0		# for QEMU
#BOOTDISK=		wd0		# for Ultra5/10 image?
PRIMARY_BOOT=		bootblk
#SECONDARY_BOOT=	# nothing
#SECONDARY_BOOT_ARG=	# unnecessary

USE_SUNLABEL=		yes

.include "${.CURDIR}/../../../common/bootimage/Makefile.liveimage"
