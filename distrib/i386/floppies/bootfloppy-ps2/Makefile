#	$NetBSD: Makefile,v 1.2 2001/07/30 21:38:22 jdolecek Exp $

TOP=	${.CURDIR}/..

.include "${TOP}/Makefile.inc"

IMAGE=	boot-ps2.fs
KERN!=	cd $(.CURDIR)/../kernel-ramdisk/; \
        printf "xxx: .MAKE\n\t@echo \$${.OBJDIR}/netbsd.INSTALL_PS2.gz\n" | \
	${MAKE} -s -f-

DISKTYPE=	floppy
DISKSIZE=	2880
BLOCK8K=	179

.include "${TOP}/bootfloppy-common/Makefile.inc"
