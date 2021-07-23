#	$NetBSD: Makefile,v 1.1 2021/07/23 21:53:34 jmcneill Exp $

LIVEIMGBASE=	NetBSD-${DISTRIBVER}-${MACHINE_ARCH}-live	# gives ${IMGBASE}.img

EMUIMAGEMB?=	1907            	# 2GB USB flash memories aren't 2GiB
LIVEIMAGEMB=	${EMUIMAGEMB}
SWAPMB=		0			# No swap
IMAGEHOSTNAME=	netbsd

FILESDIR!=	cd ${.CURDIR}/../../../utils/embedded/files ; pwd

SPEC_EXTRA=	${.CURDIR}/spec.armimage
IMGFILE_EXTRA=								\
	${FILESDIR}/ec2_init		etc/rc.d/ec2_init		\
	${FILESDIR}/resize_gpt		etc/rc.d/resize_gpt		\
	${.CURDIR}/dhcpcd		etc/rc.conf.d/dhcpcd		\
	${.CURDIR}/ec2_init		etc/rc.conf.d/ec2_init		\
	${.CURDIR}/resize_gpt		etc/rc.conf.d/resize_gpt	\
	${.CURDIR}/resize_root		etc/rc.conf.d/resize_root	\
	${.CURDIR}/sshd			etc/rc.conf.d/sshd

.include "${.CURDIR}/../Makefile.liveimage"
