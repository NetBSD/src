#	$NetBSD: Makefile,v 1.2 2020/03/02 16:04:49 roy Exp $

IMAGE=			ramdisk-zfsroot.fs
IMAGESIZE=		5000k
IMAGEDEPENDS=
MAKEDEVTARGETS=		all
CRUNCHENV=		INIT_CHROOT=1
SMALLPROG_INET6=	1

# Build as re-entrant because zpool requires libhack built this way.
CPPFLAGS+=		-D_REENTRANT

LISTS+=			${DISTRIBDIR}/common/list.zfsroot

.include "${.CURDIR}/../common/Makefile.ramdisk"
.include "${DISTRIBDIR}/common/Makefile.makedev"

LISTS+=			${.CURDIR}/list

MTREECONF+=		${DISTRIBDIR}/common/mtree.zfsroot
