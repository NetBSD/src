#	$NetBSD: Makefile.inc,v 1.1 2018/11/12 20:16:46 scole Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

# Required variables:
#  FLOPPY_NETBSD
#  FLOPPYBASE

FLOPPYSIZE=	2880
FLOPPYFILES=	boot netbsd
FLOPPYMAX=	3
FLOPPYSUFFIX=	.fs

FLOPPY_RELEASEDIR=	installation/floppy

MDEC=		${DESTDIR}/usr/mdec
KERNOBJ!=	cd ${.CURDIR}/../md-kernel && ${PRINTOBJDIR}
FLOPPY_BOOT=	${MDEC}/ofwboot
FLOPPYINSTBOOT=	"${TOOL_INSTALLBOOT} -v -m ${MACHINE} -t raw -B 17 @IMAGE@ ${MDEC}/bootxx ${FLOPPY_BOOT}"

.include "${DISTRIBDIR}/common/Makefile.tarfloppy"

.include <bsd.prog.mk>
