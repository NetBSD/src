#	$NetBSD: Makefile,v 1.4 2002/02/08 04:37:25 lukem Exp $

TOP=	${.CURDIR}/..

.include "${TOP}/Makefile.inc"

IMAGE=	boot-ps2.fs
KERN!=	cd ${TOP}/kernel-ramdisk/; \
        printf "xxx: .MAKE\n\t@echo \$${.OBJDIR}/netbsd.INSTALL_PS2.gz\n" | \
	${MAKE} -s -f-

DISKSIZE=	2880

.include "${TOP}/common/Makefile.bootfloppy"
