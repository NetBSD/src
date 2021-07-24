#	$NetBSD: Makefile,v 1.9 2021/07/24 16:13:44 jmcneill Exp $

LIVEIMGBASE=	NetBSD-${DISTRIBVER}-amd64-live	# gives ${IMGBASE}.img

EMUIMAGEMB?=	1907            	# 2GB USB flash memories aren't 2GiB
LIVEIMAGEMB=	${EMUIMAGEMB}
SWAPMB=		0			# No swap
IMAGEHOSTNAME=	netbsd

FILESDIR!=	cd ${.CURDIR}/../../../utils/embedded/files ; pwd

SPEC_EXTRA=	${.CURDIR}/spec.emuimage
RC_CONF_EXTRA=	${.CURDIR}/rc.conf.emuimage
IMGFILE_EXTRA=								\
	${FILESDIR}/ec2_init		etc/rc.d/ec2_init		\
	${FILESDIR}/resize_disklabel	etc/rc.d/resize_disklabel

.include "${.CURDIR}/../Makefile.liveimage"
