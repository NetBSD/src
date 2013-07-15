#	$NetBSD: Makefile,v 1.1 2013/07/15 00:22:10 khorben Exp $

IMAGE=		ramdisk-cgdroot.fs
IMAGESIZE=	5000k
IMAGEDEPENDS= 	
CRUNCHENV=	INIT_CHROOT=1
SMALLPROG_INET6=1

.include "${.CURDIR}/../common/Makefile.ramdisk"

LISTS+=		${DISTRIBDIR}/common/list.cgdroot
MTREECONF+=	${DISTRIBDIR}/common/mtree.cgdroot

.if ${USE_INET6} != "no"
LISTS+=		${DISTRIBDIR}/common/list.inet6
.endif
