#	$NetBSD: Makefile,v 1.6 2000/05/05 20:13:30 mycroft Exp $

TOP=	${.CURDIR}/..

.include "${TOP}/Makefile.inc"

IMAGE=	boot.fs
KERN!=	cd $(.CURDIR)/../ramdisk-kernel/; \
        printf "xxx: .MAKE\n\t@echo \$${.OBJDIR}/netbsd.INSTALL.gz\n" | \
	${MAKE} -s -f-

.include "${TOP}/bootfloppy-common/Makefile.inc"
