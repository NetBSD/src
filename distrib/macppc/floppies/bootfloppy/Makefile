#	$NetBSD: Makefile,v 1.24 2002/05/04 01:34:38 lukem Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

FLOPPYBASE=	boot
FLOPPYSIZE=	2880
FLOPPYFILES=	boot netbsd
FLOPPYMAX=	2
FLOPPYSUFFIX=	.fs

FLOPPY_RELEASEDIR=	installation/floppy

MDEC=		${DESTDIR}/usr/mdec
KERNOBJ!=	cd ${.CURDIR}/../md-kernel && ${PRINTOBJDIR}
FLOPPY_NETBSD=	${KERNOBJ}/netbsd.INSTALL.gz
FLOPPY_BOOT=	${MDEC}/ofwboot
#	XXXDISTRIB: need MI installboot for macppc
FLOPPYINSTBOOT=	"${MDEC}/installboot -v -b 17 boot ${MDEC}/bootxx @IMAGE@"

.include "${DISTRIBDIR}/common/Makefile.tarfloppy"

.include <bsd.prog.mk>
