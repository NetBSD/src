#	$NetBSD: Makefile.cats.inc,v 1.3 2002/01/31 10:37:41 chris Exp $

GENASSYM_EXTRAS+=	${ARM}/footbridge/genassym.cf
LOOSE_PROTOTYPES=	NO

.if (${OBJECT_FMT} == "ELF")
# Under ELF we need to do a whole load of stuff extra to get that kernel.

LINKADDRESS=0xF0000020

SYSTEM_LD_TAIL= \
	$S/arch/${MACHINE}/conf/elf2aout.sh
.endif	
