#	$NetBSD: Makefile,v 1.13 2002/05/14 06:21:21 lukem Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

FLOPPYBASE=	disk
FLOPPYSUFFIX=	of2
FLOPPYSIZE=	2880
FLOPPYFILES=	boot netbsd
FLOPPYMAX=	2

FLOPPY_RELEASEDIR=	installation/floppy

MDEC=		${DESTDIR}/usr/mdec
KERNOBJ!=	cd ${.CURDIR}/../instkernel && ${PRINTOBJDIR}
FLOPPY_NETBSD=	${KERNOBJ}/netbsd.gz
FLOPPY_BOOT=	${MDEC}/ustarboot
FLOPPYINSTBOOT=\
	"${INSTALLBOOT} -v -m ${MACHINE} -t raw -b 17 @IMAGE@ ${FLOPPY_BOOT}"

.include "${DISTRIBDIR}/common/Makefile.tarfloppy"

.include <bsd.prog.mk>
