#	$NetBSD: Makefile,v 1.1 2000/06/10 18:56:49 he Exp $

TOP=	${.CURDIR}/..

.include "${TOP}/Makefile.inc"

BASENAME= rescue
KDIR=	${.CURDIR}/../rescue-kernel
KNAME=	INSTALL
KERN!=	cd ${KDIR}; \
        printf "xxx: .MAKE\n\t@echo \$${.OBJDIR}/netbsd.${KNAME}.gz\n" | \
	${MAKE} -s -f-

.include "${TOP}/fdset-common/Makefile.inc"

${KERN}:
	cd ${KDIR}; ${MAKE}
