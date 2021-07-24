#	$NetBSD: Makefile,v 1.2 2021/07/24 14:00:08 jmcneill Exp $

LIVEIMGBASE=	NetBSD-${DISTRIBVER}-${MACHINE_ARCH}-live	# gives ${IMGBASE}.img

EMUIMAGEMB?=	1907            	# 2GB USB flash memories aren't 2GiB
LIVEIMAGEMB=	${EMUIMAGEMB}
SWAPMB=		0			# No swap
IMAGEHOSTNAME=	netbsd

FILESDIR!=	cd ${.CURDIR}/../../../utils/embedded/files ; pwd

SPEC_EXTRA=	${.CURDIR}/spec.armimage
RC_CONF_EXTRA=	${.CURDIR}/rc.conf.armimage
IMGFILE_EXTRA=								\
	${FILESDIR}/ec2_init		etc/rc.d/ec2_init		\
	${FILESDIR}/resize_gpt		etc/rc.d/resize_gpt

.include "${.CURDIR}/../Makefile.liveimage"
