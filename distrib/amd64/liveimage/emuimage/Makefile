#	$NetBSD: Makefile,v 1.8 2020/08/05 01:35:18 jmcneill Exp $

LIVEIMGBASE=	NetBSD-${DISTRIBVER}-amd64-live	# gives ${IMGBASE}.img

EMUIMAGEMB?=	1907            	# 2GB USB flash memories aren't 2GiB
LIVEIMAGEMB=	${EMUIMAGEMB}
SWAPMB=		0			# No swap
IMAGEHOSTNAME=	netbsd

FILESDIR!=	cd ${.CURDIR}/../../../utils/embedded/files ; pwd

SPEC_EXTRA=	${.CURDIR}/spec.emuimage
IMGFILE_EXTRA=								\
	${FILESDIR}/ec2_init		etc/rc.d/ec2_init		\
	${FILESDIR}/resize_disklabel	etc/rc.d/resize_disklabel	\
	${.CURDIR}/dhcpcd		etc/rc.conf.d/dhcpcd		\
	${.CURDIR}/ec2_init		etc/rc.conf.d/ec2_init		\
	${.CURDIR}/resize_disklabel	etc/rc.conf.d/resize_disklabel	\
	${.CURDIR}/resize_root		etc/rc.conf.d/resize_root	\
	${.CURDIR}/sshd			etc/rc.conf.d/sshd

.include "${.CURDIR}/../Makefile.liveimage"
