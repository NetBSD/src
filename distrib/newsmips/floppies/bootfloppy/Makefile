#	$NetBSD: Makefile,v 1.8 2002/04/19 10:40:23 tsutsui Exp $

.include <bsd.own.mk>
.include "${_SRC_TOP_}/distrib/Makefile.inc"

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
