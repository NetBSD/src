#	$NetBSD: Makefile,v 1.1 2020/02/22 09:53:47 roy Exp $

IMAGE=			ramdisk-zfsroot.fs
IMAGESIZE=		5000k
IMAGEDEPENDS=
MAKEDEVTARGETS=		all
CRUNCHENV=		INIT_CHROOT=1
SMALLPROG_INET6=	1

LISTS+=			${DISTRIBDIR}/common/list.zfsroot

.include "${.CURDIR}/../common/Makefile.ramdisk"
.include "${DISTRIBDIR}/common/Makefile.makedev"

LISTS+=			${.CURDIR}/list

MTREECONF+=		${DISTRIBDIR}/common/mtree.zfsroot
