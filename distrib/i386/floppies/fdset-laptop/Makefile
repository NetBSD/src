#	$NetBSD: Makefile,v 1.1 2000/09/27 16:04:14 fvdl Exp $

TOP=	${.CURDIR}/..

.include "${TOP}/Makefile.inc"

KERN!=	cd $(.CURDIR)/../kernel-ramdisk/; \
        printf "xxx: .MAKE\n\t@echo \$${.OBJDIR}/netbsd.INSTALL_LAPTOP.gz\n" | \
	${MAKE} -s -f-

.include "${TOP}/fdset-common/Makefile.inc"
