#	$NetBSD: Makefile,v 1.9 2002/04/26 15:50:41 lukem Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/Makefile.inc"

FLOPPYBASE=	boot
FLOPPYSIZE=	2880
FLOPPYFILES=	boot netbsd
FLOPPYMAX=	2
FLOPPYINSTDIR=	installation/floppy

MDEC=		${DESTDIR}/usr/mdec
KERNOBJ!=	cd ${.CURDIR}/../instkernel && ${PRINTOBJDIR}
FLOPPY_NETBSD=	${KERNOBJ}/netbsd.INSTALL.gz
FLOPPY_BOOT=	${MDEC}/boot
#	XXXDISTRIB: need MI installboot for newsmips
FLOPPYINSTBOOT=	"${MDEC}/installboot -v -b 17 boot ${MDEC}/bootxx @IMAGE@"

.include "${DISTRIBDIR}/common/Makefile.tarfloppy"

.include <bsd.prog.mk>
