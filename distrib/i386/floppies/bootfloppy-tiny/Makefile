#	$NetBSD: Makefile,v 1.5 2000/05/05 21:35:51 mycroft Exp $

TOP=	${.CURDIR}/..

.include "${TOP}/Makefile.inc"

IMAGE=	boot-tiny.fs
KERN!=	cd $(.CURDIR)/../ramdisk-kernel/; \
        printf "xxx: .MAKE\n\t@echo \$${.OBJDIR}/netbsd.INSTALL_TINY.gz\n" | \
	${MAKE} -s -f-

DISKTYPE=	floppy5
DISKSIZE=	2400
BLOCK8K=	149

.include "${TOP}/bootfloppy-common/Makefile.inc"
