#	$NetBSD: Makefile,v 1.10 2002/02/03 16:22:21 lukem Exp $

TOP=	${.CURDIR}/..

.include "${TOP}/Makefile.inc"

IMAGE=	boot-small.fs
KERN!=	cd $(.CURDIR)/../kernel-ramdisk/; \
        printf "xxx: .MAKE\n\t@echo \$${.OBJDIR}/netbsd.INSTALL_SMALL.gz\n" | \
	${MAKE} -s -f-

DISKSIZE=	2400

.include "${TOP}/common/Makefile.bootfloppy"
