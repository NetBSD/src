#	$NetBSD: Makefile,v 1.7 2000/05/06 05:32:29 mycroft Exp $

TOP=	${.CURDIR}/..

.include "${TOP}/Makefile.inc"

IMAGE=	boot-small.fs
KERN!=	cd $(.CURDIR)/../ramdisk-kernel/; \
        printf "xxx: .MAKE\n\t@echo \$${.OBJDIR}/netbsd.INSTALL_SMALL.gz\n" | \
	${MAKE} -s -f-

DISKTYPE=	floppy3
DISKSIZE=	2880
BLOCK8K=	179

.include "${TOP}/bootfloppy-common/Makefile.inc"
