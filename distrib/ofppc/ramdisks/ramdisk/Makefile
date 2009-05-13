#	$NetBSD: Makefile,v 1.1.16.1 2009/05/13 19:17:49 jym Exp $

IMAGE=		ramdisk.fs
IMAGESIZE=	5000k
USE_SYSINST=	yes
SMALLPROG_INET6=1

.include "${.CURDIR}/../common/Makefile.ramdisk"

.if ${USE_INET6} != "no"
LISTS+=		${.CURDIR}/list.inet6
.endif
