#	$NetBSD: Makefile,v 1.2 2003/05/15 14:02:37 fvdl Exp $

IMAGE=		ramdisk.fs
IMAGESIZE=	2496k
IMAGEDEPENDS= 	disktab.preinstall \
		${NETBSDSRCDIR}/usr.sbin/dhcp/clientscript/dhclient-script \
		${DESTDIR}/usr/mdec/biosboot_com0.sym
USE_SYSINST=	yes
SMALLPROG_INET6=1

.include "${.CURDIR}/../common/Makefile.ramdisk"
