#	$NetBSD: Makefile,v 1.5 2003/11/10 00:43:20 fvdl Exp $

IMAGE=		ramdisk.fs
IMAGESIZE=	2304k
IMAGEDEPENDS= 	disktab.preinstall \
		${NETBSDSRCDIR}/usr.sbin/dhcp/clientscript/dhclient-script
USE_SYSINST=	yes
SMALLPROG_INET6=1

.include "${.CURDIR}/../common/Makefile.ramdisk"
