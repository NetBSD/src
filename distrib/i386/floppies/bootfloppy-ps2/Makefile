#	$NetBSD: Makefile,v 1.1 2001/07/21 14:46:09 jdolecek Exp $

TOP=	${.CURDIR}/..

.include "${TOP}/Makefile.inc"

IMAGE=	boot-ps2.fs
KERN!=	cd $(.CURDIR)/../kernel-ramdisk/; \
        printf "xxx: .MAKE\n\t@echo \$${.OBJDIR}/netbsd.INSTALL_PS2.gz\n" | \
	${MAKE} -s -f-

DISKTYPE=	floppy
DISKSIZE=	2880
BLOCK8K=	149

.include "${TOP}/bootfloppy-common/Makefile.inc"
