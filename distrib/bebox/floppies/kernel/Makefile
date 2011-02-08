#	$NetBSD: Makefile,v 1.1.2.1 2011/02/08 16:18:33 bouyer Exp $
#
# Makefile snippet to create a set of USTAR floppies
#

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

FLOPPYBASE=		kernel
FLOPPYSIZE=		2880
FLOPPYFILES=		netbsd

FLOPPYDIR!=  		cd ${.CURDIR}/../kernel-ramdisk && ${PRINTOBJDIR}
FLOPPY_NETBSD=		${FLOPPYDIR}/netbsd-INSTALL.gz
FLOPPYPAD=		1
FLOPPYSUFFIX=		.fs

FLOPPY_RELEASEDIR=	installation/floppy

.include "${DISTRIBDIR}/common/Makefile.tarfloppy"

.include <bsd.prog.mk>
