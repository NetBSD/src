#	$NetBSD: Makefile,v 1.5 2002/02/26 22:52:52 lukem Exp $

TOP=	${.CURDIR}/..

.include "${TOP}/Makefile.inc"

BASENAME=bootlap
KERN!=	cd ${TOP}/kernel-ramdisk/; \
        printf "xxx: .MAKE\n\t@echo \$${.OBJDIR}/netbsd.INSTALL_LAPTOP.gz\n" | \
	${MAKE} -s -f-

.include "${TOP}/common/Makefile.fdset"
