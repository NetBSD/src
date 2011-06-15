#	$NetBSD: bsd.powerpc-4xx.mk,v 1.1 2011/06/15 09:45:59 mrg Exp $

.ifndef _BSD_POWERPC_4XX_MK_
_BSD_POWERPC_4XX_MK_=1

KMODULEARCHDIR:=	powerpc-4xx

CPPFLAGS+=	-mcpu=403
PPC_IBM4XX=	1

# hack into bsd.kmodule.mk
PPC_INTR_IMPL=\"powerpc/ibm4xx/ibm4xx_intr.h\"
PPC_PCI_MACHDEP_IMPL=\"powerpc/ibm4xx/pci_machdep.h\"

AFLAGS+=	-mcpu=403

.endif # _BSD_POWERPC_4XX_MK_
