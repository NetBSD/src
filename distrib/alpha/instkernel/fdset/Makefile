#	$NetBSD: Makefile,v 1.11 2002/05/04 01:34:37 lukem Exp $

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
FLOPPY_NETBSD=	${KERNOBJ}/netbsd.INSTALL.gz
FLOPPY_BOOT=	${MDEC}/ustarboot
FLOPPYINSTBOOT=\
	"${INSTALLBOOT} -m ${MACHINE} -t raw -v -b 17 @IMAGE@ ${FLOPPY_BOOT}"

.include "${DISTRIBDIR}/common/Makefile.tarfloppy"

.include <bsd.prog.mk>
