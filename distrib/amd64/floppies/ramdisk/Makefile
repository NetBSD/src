#	$NetBSD: Makefile,v 1.7 2005/01/19 00:16:17 lukem Exp $

IMAGE=		ramdisk.fs
IMAGESIZE=	2304k
IMAGEDEPENDS= 	disktab.preinstall \
		${NETBSDSRCDIR}/usr.sbin/dhcp/clientscript/dhclient-script
USE_SYSINST=	yes
SMALLPROG_INET6=1

.include "${.CURDIR}/../common/Makefile.ramdisk"

.if ${USE_INET6} != "no"
LISTS+=		${.CURDIR}/list.inet6
.endif
