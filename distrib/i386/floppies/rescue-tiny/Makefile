#	$NetBSD: Makefile,v 1.3.2.2 2000/10/09 13:43:34 fvdl Exp $

TOP=	${.CURDIR}/..

.include "${TOP}/Makefile.inc"

IMAGE=	rescue-tiny.fs
KDIR=	${.CURDIR}/../kernel-rescue
KNAME=	INSTALL_TINY
KERN!=	cd ${KDIR}; \
        printf "xxx: .MAKE\n\t@echo \$${.OBJDIR}/netbsd.${KNAME}.gz\n" | \
	${MAKE} -s -f-

DISKTYPE=	floppy5
DISKSIZE=	2400
BLOCK8K=	149

.include "${TOP}/bootfloppy-common/Makefile.inc"

${KERN}:
	cd ${KDIR}; ${MAKE}
