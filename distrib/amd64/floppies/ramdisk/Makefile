#	$NetBSD: Makefile,v 1.4 2003/08/30 20:08:14 fvdl Exp $

IMAGE=		ramdisk.fs
IMAGESIZE=	2560k
IMAGEDEPENDS= 	disktab.preinstall \
		${NETBSDSRCDIR}/usr.sbin/dhcp/clientscript/dhclient-script
USE_SYSINST=	yes
SMALLPROG_INET6=1

.include "${.CURDIR}/../common/Makefile.ramdisk"
