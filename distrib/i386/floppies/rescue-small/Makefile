#	$NetBSD: Makefile,v 1.4 2002/02/03 16:22:29 lukem Exp $

TOP=	${.CURDIR}/..

.include "${TOP}/Makefile.inc"

IMAGE=	rescue-small.fs
KDIR=	${.CURDIR}/../kernel-rescue
KNAME=	INSTALL_SMALL
KERN!=	cd ${KDIR}; \
        printf "xxx: .MAKE\n\t@echo \$${.OBJDIR}/netbsd.${KNAME}.gz\n" | \
	${MAKE} -s -f-

DISKSIZE=	2400

.include "${TOP}/common/Makefile.bootfloppy"

${KERN}:
	cd ${KDIR}; ${MAKE}
