#	$NetBSD: Makefile,v 1.16 2003/07/10 10:34:09 lukem Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

FLOPPYBASE=	boot
FLOPPYSIZE=	2880
FLOPPYFILES=	boot netbsd
FLOPPYMAX=	2
FLOPPYSUFFIX=	.fs

FLOPPY_RELEASEDIR=	installation/floppy

MDEC=		${DESTDIR}/usr/mdec
KERNOBJ!=	cd ${.CURDIR}/../instkernel && ${PRINTOBJDIR}
FLOPPY_NETBSD=	${KERNOBJ}/netbsd-INSTALL.gz
FLOPPY_BOOT=	${MDEC}/boot
FLOPPYINSTBOOT= "${TOOL_INSTALLBOOT} -m ${MACHINE} -t raw -v -B 17 @IMAGE@ ${MDEC}/bootxx boot"

.include "${DISTRIBDIR}/common/Makefile.tarfloppy"

.include <bsd.prog.mk>
