#	$NetBSD: Makefile,v 1.1 2002/04/16 05:23:38 lukem Exp $

FLOPPYBASE=	boot-com
FLOPPYMAX=	2
FLOPPYKERNDIR=	kernel-ramdisk
FLOPPYKERNEL=	netbsd.INSTALL.gz
FLOPPYBOOTCODE=	${DESTDIR}/usr/mdec/biosboot_com0.sym	# Serial output

.include "${.CURDIR}/../common/Makefile.bootfloppy"
