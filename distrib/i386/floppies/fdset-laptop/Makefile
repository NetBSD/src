#	$NetBSD: Makefile,v 1.2 2000/09/27 19:13:52 fvdl Exp $

TOP=	${.CURDIR}/..

.include "${TOP}/Makefile.inc"

KERN!=	cd $(.CURDIR)/../kernel-ramdisk/; \
        printf "xxx: .MAKE\n\t@echo \$${.OBJDIR}/netbsd.INSTALL_LAPTOP.gz\n" | \
	${MAKE} -s -f-
BASENAME=bootlap

.include "${TOP}/fdset-common/Makefile.inc"
