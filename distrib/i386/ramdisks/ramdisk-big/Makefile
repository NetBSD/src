#	$NetBSD: Makefile,v 1.1.30.1 2009/05/13 19:17:43 jym Exp $

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
