#	$NetBSD: Makefile,v 1.1 2012/01/22 03:53:31 tsutsui Exp $

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
