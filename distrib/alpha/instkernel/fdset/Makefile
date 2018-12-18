#	$NetBSD: Makefile,v 1.16 2018/12/18 12:55:38 martin Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

FLOPPYBASE=	disk
FLOPPYSUFFIX=	of4
FLOPPYSIZE=	2880
FLOPPYFILES=	boot netbsd
FLOPPYMAX=	4

FLOPPY_RELEASEDIR=	installation/floppy

MDEC=		${DESTDIR}/usr/mdec
KERNOBJ!=	cd ${.CURDIR}/../instkernel && ${PRINTOBJDIR}
FLOPPY_NETBSD=	${KERNOBJ}/netbsd.gz
FLOPPY_BOOT=	${MDEC}/ustarboot
FLOPPYINSTBOOT=	"${TOOL_INSTALLBOOT} -v -m ${MACHINE} -t raw -b 17 @IMAGE@ ${FLOPPY_BOOT}"

.include "${DISTRIBDIR}/common/Makefile.tarfloppy"

.include <bsd.prog.mk>
