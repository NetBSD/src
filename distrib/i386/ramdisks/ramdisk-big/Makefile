#	$NetBSD: Makefile,v 1.3 2009/09/20 22:46:12 abs Exp $

BOOTMODEL=	big
IMAGE=		ramdisk-${BOOTMODEL}.fs
IMAGESIZE=	5000k
IMAGEDEPENDS= 	
USE_SYSINST=	yes
SMALLPROG_INET6=1

.include "${.CURDIR}/../common/Makefile.ramdisk"
.include "${DISTRIBDIR}/common/Makefile.dhcpcd"

.if ${USE_INET6} != "no"
LISTS+=		${.CURDIR}/list.inet6
.endif

MAKEFS_FLAGS+=	-f 20
