#	$NetBSD: Makefile,v 1.27 2021/06/18 20:30:58 mrg Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"
.include <bsd.kernobj.mk>

IMAGE=		ramdisk.fs
IMAGESIZE=	600k
MAKEFS_FLAGS+=	-f 15 -o density=3072,bsize=4096,fsize=512,optimization=space

WARNS=		1
DBG=		-Os -fno-unwind-tables -fno-ident

# XXXMRG - see if these help/hinder reduced size?
#-fno-jump-tables 
#-fzero-initialized-in-bss
#-fgcse-after-reload
#-fdelete-null-pointer-checks
#-fno-sched-interblock
#-fno-sched-spec
#-fsched-pressure
#-fcode-hoisting
#-ftree-partial-pre
#-fno-ipa-cp
#-ftree-builtin-call-dce
#-fno-align-functions
#-fno-align-loops
#-fno-align-jumps
#-fno-align-labels
#-fallow-store-data-races

CRUNCHBIN=	rd_bin
LISTS=		${.CURDIR}/list
MTREECONF=	${.CURDIR}/mtree.conf
IMAGEENDIAN=	be
MAKEDEVTARGETS=	std md0 cd0 st0 st1 sd0 sd1 sd2 xd0 xy0
IMAGEDEPENDS=	${CRUNCHBIN} \
		fstab passwd protocols ${DISTRIBDIR}/common/services \
		sshrc welcome ${NETBSDSRCDIR}/etc/netconfig

MDSETTARGETS=		RAMDISK ramdisk.fs -
MDSET_RELEASEDIR=	binary/kernel

# Use stubs to eliminate some large stuff from libc
HACKSRC=	${DISTRIBDIR}/utils/libhack
.MAKEFLAGS+=	NOIEEE80211=1
.include	"${HACKSRC}/Makefile.inc"
${CRUNCHBIN}:	libhack.o

fstab: .PHONY

.include "${DISTRIBDIR}/common/Makefile.crunch"
.include "${DISTRIBDIR}/common/Makefile.makedev"
.include "${DISTRIBDIR}/common/Makefile.image"
.include "${DISTRIBDIR}/common/Makefile.mdset"

.include <bsd.prog.mk>
