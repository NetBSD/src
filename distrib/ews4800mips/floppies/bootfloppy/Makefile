#	$NetBSD: Makefile,v 1.1 2005/12/29 16:01:30 tsutsui Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

FLOPPYBASE=	boot
FLOPPYSIZE=	1989	# 2D excluding cylinder 0 side 0
FLOPPYFILES=	boot netbsd
FLOPPYMAX=	2
FLOPPYSUFFIX=	.fs

FLOPPY_RELEASEDIR=	installation/floppy

MDEC=		${DESTDIR}/usr/mdec
KERNOBJ!=	cd ${.CURDIR}/../instkernel && ${PRINTOBJDIR}
FLOPPY_BOOT=	${MDEC}/boot
FLOPPY_NETBSD=	${KERNOBJ}/netbsd-RAMDISK.gz
FLOPPYINSTBOOT=	"dd if=${MDEC}/bootxx_ustarfs of=@IMAGE@ conv=notrunc"

.include "${DISTRIBDIR}/common/Makefile.tarfloppy"

.include <bsd.prog.mk>
