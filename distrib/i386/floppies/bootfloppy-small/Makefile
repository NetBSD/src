#	$NetBSD: Makefile,v 1.5 2000/05/05 20:13:31 mycroft Exp $

TOP=	${.CURDIR}/..

.include "${TOP}/Makefile.inc"

IMAGE=	boot-small.fs
KERN!=	cd $(.CURDIR)/../ramdisk-kernel/; \
        printf "xxx: .MAKE\n\t@echo \$${.OBJDIR}/netbsd.INSTALL_SMALL.gz\n" | \
	${MAKE} -s -f-

DISKTYPE=	floppy5
DISKSIZE=	2400

.include "${TOP}/bootfloppy-common/Makefile.inc"
