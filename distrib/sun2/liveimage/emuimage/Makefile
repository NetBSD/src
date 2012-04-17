#	$NetBSD: Makefile,v 1.1.4.2 2012/04/17 00:02:48 yamt Exp $

LIVEIMGBASE=	sun2emu			# gives ${IMGBASE}.img

.include <bsd.own.mk>

KERN_SET=		kern-GENERIC
BOOTDISK=		sd0
PRIMARY_BOOT=		bootxx
SECONDARY_BOOT=		ufsboot
SECONDARY_BOOT_ARG=	/${SECONDARY_BOOT}

#USE_SUNLABEL=		yes		# sun2 PROM doesn't read sunlabel

.include "${.CURDIR}/../../../common/bootimage/Makefile.liveimage"
