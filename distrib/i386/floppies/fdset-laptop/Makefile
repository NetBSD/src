#	$NetBSD: Makefile,v 1.4 2002/02/08 04:40:07 lukem Exp $

TOP=	${.CURDIR}/..

.include "${TOP}/Makefile.inc"

KERN!=	cd ${TOP}/kernel-ramdisk/; \
        printf "xxx: .MAKE\n\t@echo \$${.OBJDIR}/netbsd.INSTALL_LAPTOP.gz\n" | \
	${MAKE} -s -f-
BASENAME=bootlap

.include "${TOP}/common/Makefile.fdset"
