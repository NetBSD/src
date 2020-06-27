#	$NetBSD: bsd.powerpc-ibm4xx.mk,v 1.1 2020/06/27 06:50:00 rin Exp $

.ifndef _BSD_POWERPC_IBM4XX_MK_
_BSD_POWERPC_IBM4XX_MK_=1

KMODULEARCHDIR:=	powerpc-ibm4xx

CPPFLAGS+=	-mcpu=403
PPC_IBM4XX=	1

# hack into bsd.kmodule.mk
PPC_INTR_IMPL=\"powerpc/intr.h\"
PPC_PCI_MACHDEP_IMPL=\"powerpc/pci_machdep.h\"

AFLAGS+=	-mcpu=403

.endif # _BSD_POWERPC_IBM4XX_MK_
