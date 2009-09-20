#	$NetBSD: Makefile,v 1.3 2009/09/20 22:50:51 abs Exp $

IMAGE=		ramdisk.fs
IMAGESIZE=	5000k
USE_SYSINST=	yes
SMALLPROG_INET6=1

.include "${.CURDIR}/../common/Makefile.ramdisk"

.if ${USE_INET6} != "no"
LISTS+=		${DISTRIBDIR}/common/list.inet6
.endif
