#	$NetBSD: Makefile,v 1.2 2009/04/07 11:49:17 joerg Exp $

BOOTMODEL=	big
IMAGE=		ramdisk-${BOOTMODEL}.fs
IMAGESIZE=	5000k
IMAGEDEPENDS= 	disktab.preinstall
USE_SYSINST=	yes
SMALLPROG_INET6=1

.include "${.CURDIR}/../common/Makefile.ramdisk"
.include "${DISTRIBDIR}/common/Makefile.dhcpcd"

.if ${USE_INET6} != "no"
LISTS+=		${.CURDIR}/list.inet6
.endif

MAKEFS_FLAGS+=	-f 20
