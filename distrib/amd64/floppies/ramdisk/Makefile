#	$NetBSD: Makefile,v 1.8 2005/11/27 17:39:01 dsl Exp $

IMAGE=		ramdisk.fs
IMAGESIZE=	5000k
IMAGEDEPENDS= 	disktab.preinstall \
		${NETBSDSRCDIR}/usr.sbin/dhcp/clientscript/dhclient-script
USE_SYSINST=	yes
SMALLPROG_INET6=1

.include "${.CURDIR}/../common/Makefile.ramdisk"

.if ${USE_INET6} != "no"
LISTS+=		${.CURDIR}/list.inet6
.endif
