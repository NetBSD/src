#	$NetBSD: Makefile,v 1.3 2002/02/03 16:22:23 lukem Exp $

TOP=	${.CURDIR}/..

.include "${TOP}/Makefile.inc"

KERN!=	cd $(.CURDIR)/../kernel-ramdisk/; \
        printf "xxx: .MAKE\n\t@echo \$${.OBJDIR}/netbsd.INSTALL_LAPTOP.gz\n" | \
	${MAKE} -s -f-
BASENAME=bootlap

.include "${TOP}/common/Makefile.fdset"
