#	$NetBSD: Makefile,v 1.7 2022/05/22 19:32:41 jmcneill Exp $

.include <bsd.own.mk>

INSTIMGBASE=	NetBSD-${DISTRIBVER}-amd64-bios-install	# gives ${IMGBASE}.img

.include "../installimage/installimage.mk"

USE_MBR=		yes

.include "${NETBSDSRCDIR}/distrib/common/bootimage/Makefile.installimage"
