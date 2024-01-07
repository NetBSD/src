#	$NetBSD: Makefile,v 1.1 2024/01/07 00:57:13 thorpej Exp $

IMAGE=		inst-ramdisk.fs
IMAGESIZE=	4000k
IMAGEDEPENDS= 	
USE_SYSINST=	yes
SMALLPROG_INET6=1

.include "${.CURDIR}/../common/Makefile.ramdisk"

.if ${USE_INET6} != "no"
LISTS+=		${DISTRIBDIR}/common/list.inet6
.endif
