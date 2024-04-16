#	$NetBSD: Makefile,v 1.10 2024/04/16 16:13:44 maya Exp $

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
	${FILESDIR}/resize_gpt		etc/rc.d/resize_gpt

.include "${.CURDIR}/../Makefile.liveimage"
