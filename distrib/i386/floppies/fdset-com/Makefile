#	$NetBSD: Makefile,v 1.1 2000/12/14 22:26:06 jmc Exp $

TOP=	${.CURDIR}/..

.include "${TOP}/Makefile.inc"

BASENAME=boot-com
BOOTCODE=      ${MDEC}/biosboot_com0.sym       # Serial output

KERN!=	cd $(.CURDIR)/../kernel-ramdisk/; \
        printf "xxx: .MAKE\n\t@echo \$${.OBJDIR}/netbsd.INSTALL.gz\n" | \
	${MAKE} -s -f-

.include "${TOP}/fdset-common/Makefile.inc"
