#	$NetBSD: Makefile,v 1.2.2.2 2000/10/09 13:43:29 fvdl Exp $

TOP=	${.CURDIR}/..

.include "${TOP}/Makefile.inc"

KERN!=	cd $(.CURDIR)/../kernel-ramdisk/; \
        printf "xxx: .MAKE\n\t@echo \$${.OBJDIR}/netbsd.INSTALL_LAPTOP.gz\n" | \
	${MAKE} -s -f-
BASENAME=bootlap

.include "${TOP}/fdset-common/Makefile.inc"
