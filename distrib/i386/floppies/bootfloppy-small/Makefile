#	$NetBSD: Makefile,v 1.11 2002/02/08 04:37:25 lukem Exp $

TOP=	${.CURDIR}/..

.include "${TOP}/Makefile.inc"

IMAGE=	boot-small.fs
KERN!=	cd ${TOP}/kernel-ramdisk/; \
        printf "xxx: .MAKE\n\t@echo \$${.OBJDIR}/netbsd.INSTALL_SMALL.gz\n" | \
	${MAKE} -s -f-

DISKSIZE=	2400

.include "${TOP}/common/Makefile.bootfloppy"
