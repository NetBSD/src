#	$NetBSD: Makefile,v 1.1 2000/06/10 18:56:49 he Exp $

TOP=	${.CURDIR}/..

.include "${TOP}/Makefile.inc"

IMAGE=	rescue-sm.fs
KDIR=	${.CURDIR}/../rescue-kernel
KNAME=	INSTALL_SMALL
KERN!=	cd ${KDIR}; \
        printf "xxx: .MAKE\n\t@echo \$${.OBJDIR}/netbsd.${KNAME}.gz\n" | \
	${MAKE} -s -f-

DISKTYPE=	floppy3
DISKSIZE=	2880
BLOCK8K=	179

.include "${TOP}/bootfloppy-common/Makefile.inc"

${KERN}:
	cd ${KDIR}; ${MAKE}
