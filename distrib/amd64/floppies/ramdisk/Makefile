#	$NetBSD: Makefile,v 1.3 2003/07/26 08:32:19 dsl Exp $

IMAGE=		ramdisk.fs
IMAGESIZE=	2496k
IMAGEDEPENDS= 	disktab.preinstall \
		${NETBSDSRCDIR}/usr.sbin/dhcp/clientscript/dhclient-script
USE_SYSINST=	yes
SMALLPROG_INET6=1

.include "${.CURDIR}/../common/Makefile.ramdisk"
