#	$NetBSD: Makefile,v 1.6 2005/01/18 12:38:45 lukem Exp $

IMAGE=		ramdisk.fs
IMAGESIZE=	2304k
IMAGEDEPENDS= 	disktab.preinstall \
		${NETBSDSRCDIR}/usr.sbin/dhcp/clientscript/dhclient-script
USE_SYSINST=	yes
SMALLPROG_INET6=1

.if ${USE_INET6} != "no"
LISTS+=		${.CURDIR}/list.inet6
.endif

.include "${.CURDIR}/../common/Makefile.ramdisk"
