#	$NetBSD: Makefile,v 1.3 2002/02/03 16:22:21 lukem Exp $

TOP=	${.CURDIR}/..

.include "${TOP}/Makefile.inc"

IMAGE=	boot-ps2.fs
KERN!=	cd $(.CURDIR)/../kernel-ramdisk/; \
        printf "xxx: .MAKE\n\t@echo \$${.OBJDIR}/netbsd.INSTALL_PS2.gz\n" | \
	${MAKE} -s -f-

DISKSIZE=	2880

.include "${TOP}/common/Makefile.bootfloppy"
