#	$NetBSD: Makefile,v 1.1.4.2 2008/01/02 11:39:05 mrg Exp $

IMAGE=		ramdisk.fs
IMAGESIZE=	5000k
IMAGEDEPENDS= 	${NETBSDSRCDIR}/usr.sbin/dhcp/clientscript/dhclient-script
USE_SYSINST=	yes
SMALLPROG_INET6=1

.include "${.CURDIR}/../common/Makefile.ramdisk"

.if ${USE_INET6} != "no"
LISTS+=		${.CURDIR}/list.inet6
.endif
