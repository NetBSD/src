#	$NetBSD: Makefile,v 1.1 2003/04/26 18:55:44 fvdl Exp $

IMAGE=		ramdisk.fs
IMAGESIZE=	2560k
IMAGEDEPENDS= 	disktab.preinstall \
		${NETBSDSRCDIR}/usr.sbin/dhcp/clientscript/dhclient-script \
		${DESTDIR}/usr/mdec/biosboot_com0.sym
USE_SYSINST=	yes
SMALLPROG_INET6=1

.include "${.CURDIR}/../common/Makefile.ramdisk"
