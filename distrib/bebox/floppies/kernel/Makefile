#	$NetBSD: Makefile,v 1.1 2010/10/20 02:59:58 kiyohara Exp $
#
# Makefile snippet to create a set of USTAR floppies
#

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

FLOPPYBASE=		kernel
FLOPPYSIZE=		2880
FLOPPYFILES=		netbsd

FLOPPY_NETBSD=		../kernel-ramdisk/netbsd-INSTALL.gz
FLOPPYPAD=		1
FLOPPYSUFFIX=		.fs

FLOPPY_RELEASEDIR=	installation/floppy

.include "${DISTRIBDIR}/common/Makefile.tarfloppy"

.include <bsd.prog.mk>
