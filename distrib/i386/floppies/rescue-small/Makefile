#	$NetBSD: Makefile,v 1.3 2000/09/27 16:04:17 fvdl Exp $

TOP=	${.CURDIR}/..

.include "${TOP}/Makefile.inc"

IMAGE=	rescue-small.fs
KDIR=	${.CURDIR}/../kernel-rescue
KNAME=	INSTALL_SMALL
KERN!=	cd ${KDIR}; \
        printf "xxx: .MAKE\n\t@echo \$${.OBJDIR}/netbsd.${KNAME}.gz\n" | \
	${MAKE} -s -f-

DISKTYPE=	floppy5
DISKSIZE=	2400
BLOCK8K=	149

.include "${TOP}/bootfloppy-common/Makefile.inc"

${KERN}:
	cd ${KDIR}; ${MAKE}
