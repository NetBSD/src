#	$NetBSD: Makefile,v 1.6 2002/02/03 16:22:22 lukem Exp $

TOP=	${.CURDIR}/..

.include "${TOP}/Makefile.inc"

KERN!=	cd $(.CURDIR)/../kernel-ramdisk/; \
        printf "xxx: .MAKE\n\t@echo \$${.OBJDIR}/netbsd.INSTALL.gz\n" | \
	${MAKE} -s -f-

.include "${TOP}/common/Makefile.fdset"
