#	$NetBSD: Makefile,v 1.1.2.2 2001/10/27 20:00:38 he Exp $

TOP=	${.CURDIR}/..

.include "${TOP}/Makefile.inc"

BASENAME=boot-com
BOOTCODE=      ${MDEC}/biosboot_com0.sym       # Serial output

KERN!=	cd $(.CURDIR)/../kernel-ramdisk/; \
        printf "xxx: .MAKE\n\t@echo \$${.OBJDIR}/netbsd.INSTALL.gz\n" | \
	${MAKE} -s -f-

.include "${TOP}/fdset-common/Makefile.inc"
