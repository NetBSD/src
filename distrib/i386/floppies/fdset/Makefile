#	$NetBSD: Makefile,v 1.7 2002/02/08 04:40:07 lukem Exp $

TOP=	${.CURDIR}/..

.include "${TOP}/Makefile.inc"

KERN!=	cd ${TOP}/kernel-ramdisk/; \
        printf "xxx: .MAKE\n\t@echo \$${.OBJDIR}/netbsd.INSTALL.gz\n" | \
	${MAKE} -s -f-

.include "${TOP}/common/Makefile.fdset"
