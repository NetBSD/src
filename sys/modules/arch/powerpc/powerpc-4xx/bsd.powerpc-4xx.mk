#	$NetBSD: bsd.powerpc-4xx.mk,v 1.3 2011/06/22 18:17:17 matt Exp $

.ifndef _BSD_POWERPC_4XX_MK_
_BSD_POWERPC_4XX_MK_=1

KMODULEARCHDIR:=	powerpc-4xx

CPPFLAGS+=	-mcpu=403
PPC_IBM4XX=	1

# hack into bsd.kmodule.mk
PPC_INTR_IMPL=\"powerpc/intr.h\"
PPC_PCI_MACHDEP_IMPL=\"powerpc/pci_machdep.h\"

AFLAGS+=	-mcpu=403

.endif # _BSD_POWERPC_4XX_MK_
