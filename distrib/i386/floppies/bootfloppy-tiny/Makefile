#	$NetBSD: Makefile,v 1.4 2000/05/05 20:13:31 mycroft Exp $

TOP=	${.CURDIR}/..

.include "${TOP}/Makefile.inc"

IMAGE=	boot-tiny.fs
KERN!=	cd $(.CURDIR)/../ramdisk-kernel/; \
        printf "xxx: .MAKE\n\t@echo \$${.OBJDIR}/netbsd.INSTALL_TINY.gz\n" | \
	${MAKE} -s -f-

DISKTYPE=	floppy5
DISKSIZE=	2400

.include "${TOP}/bootfloppy-common/Makefile.inc"
