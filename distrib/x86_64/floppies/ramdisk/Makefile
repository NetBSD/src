#	$NetBSD: Makefile,v 1.1 2002/12/05 02:04:13 fvdl Exp $

IMAGE=		ramdisk.fs
IMAGESIZE=	2560k
IMAGEDEPENDS= 	disktab.preinstall \
		${NETBSDSRCDIR}/usr.sbin/dhcp/clientscript/dhclient-script \
		${DESTDIR}/usr/mdec/biosboot_com0.sym
USE_SYSINST=	yes
SMALLPROG_INET6=1

.include "${.CURDIR}/../common/Makefile.ramdisk"
