#	$NetBSD: bsd.powerpc-booke.mk,v 1.1.12.1 2014/08/20 00:04:32 tls Exp $

.ifndef _BSD_POWERPC_BOOKE_MK_
_BSD_POWERPC_BOOKE_MK_=1

KMODULEARCHDIR:=	powerpc-booke

# gcc emits bad code with these options
#CPPFLAGS+=	-mcpu=8548
CPPFLAGS+=	${${ACTIVE_CC} == "gcc":? -misel -Wa,-me500 :}
PPC_BOOKE=	1

# hack into bsd.kmodule.mk
PPC_INTR_IMPL=\"powerpc/booke/intr.h\"
PPC_PCI_MACHDEP_IMPL=\"powerpc/pci_machdep.h\"

AFLAGS+=	${${ACTIVE_CC} == "gcc":? -Wa,-me500 :}

.endif # _BSD_POWERPC_BOOKE_MK_
