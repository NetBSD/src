#	$NetBSD: Makefile,v 1.22 2003/07/26 07:52:03 dsl Exp $

BOOTMODEL=	big
IMAGE=		ramdisk-${BOOTMODEL}.fs
IMAGESIZE=	2400k
IMAGEDEPENDS= 	disktab.preinstall \
		${NETBSDSRCDIR}/usr.sbin/dhcp/clientscript/dhclient-script
USE_SYSINST=	yes
SMALLPROG_INET6=1

.include "${.CURDIR}/../common/Makefile.ramdisk"

MAKEFS_FLAGS+=	-f 20
