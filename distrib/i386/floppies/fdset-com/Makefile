#	$NetBSD: Makefile,v 1.2 2002/02/03 16:22:23 lukem Exp $

TOP=	${.CURDIR}/..

.include "${TOP}/Makefile.inc"

BASENAME=boot-com
BOOTCODE=      ${MDEC}/biosboot_com0.sym       # Serial output

KERN!=	cd $(.CURDIR)/../kernel-ramdisk/; \
        printf "xxx: .MAKE\n\t@echo \$${.OBJDIR}/netbsd.INSTALL.gz\n" | \
	${MAKE} -s -f-

.include "${TOP}/common/Makefile.fdset"
