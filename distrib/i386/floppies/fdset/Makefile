#	$NetBSD: Makefile,v 1.4 2000/05/05 20:13:32 mycroft Exp $

TOP=	${.CURDIR}/..

.include "${TOP}/Makefile.inc"

KERN!=	cd $(.CURDIR)/../ramdisk-kernel/; \
        printf "xxx: .MAKE\n\t@echo \$${.OBJDIR}/netbsd.INSTALL.gz\n" | \
	${MAKE} -s -f-

.include "${TOP}/fdset-common/Makefile.inc"
