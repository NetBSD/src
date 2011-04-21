#	$NetBSD: Makefile.inc,v 1.1.2.2 2011/04/21 01:41:18 rmind Exp $

APPC=	${SYSDIR}/arch/powerpc/booke/*.S
APPC+=	${SYSDIR}/arch/powerpc/ibm4xx/*.S
APPC+=	${SYSDIR}/arch/powerpc/oea/*.S
APPC+=	${SYSDIR}/arch/powerpc/ibm4xx/openbios/*.S
APPC+=	${SYSDIR}/arch/powerpc/powerpc/*.S

SPPC=	${SYSDIR}/arch/powerpc/include/*.h
SPPC+=	${SYSDIR}/arch/powerpc/booke/*.[ch]
SPPC+=	${SYSDIR}/arch/powerpc/fpu/*.[ch]
SPPC+=	${SYSDIR}/arch/powerpc/ibm4xx/*.[ch]
SPPC+=	${SYSDIR}/arch/powerpc/isa/*.[ch]
SPPC+=	${SYSDIR}/arch/powerpc/marvell/*.[ch]
SPPC+=	${SYSDIR}/arch/powerpc/oea/*.[ch]
SPPC+=	${SYSDIR}/arch/powerpc/pci/*.[ch]
SPPC+=	${SYSDIR}/arch/powerpc/pic/*.[ch]
SPPC+=	${SYSDIR}/arch/powerpc/booke/dev/*.[ch]
SPPC+=	${SYSDIR}/arch/powerpc/booke/pci/*.[ch]
SPPC+=	${SYSDIR}/arch/powerpc/ibm4xx/dev/*.[ch]
SPPC+=	${SYSDIR}/arch/powerpc/ibm4xx/openbios/*.[ch]
SPPC+=	${SYSDIR}/arch/powerpc/ibm4xx/pci/*.[ch]
SPPC+=	${SYSDIR}/arch/powerpc/include/booke/*.[ch]
SPPC+=	${SYSDIR}/arch/powerpc/include/oea/*.[ch]
SPPC+=	${SYSDIR}/arch/powerpc/include/ibm4xx/*.[ch]
SPPC+=	${SYSDIR}/arch/powerpc/powerpc/*.[ch]
