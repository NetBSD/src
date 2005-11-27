#	$NetBSD: Makefile,v 1.7.2.1 2005/11/27 22:58:41 riz Exp $

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
