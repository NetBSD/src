#	$NetBSD: Makefile.cats.inc,v 1.4 2002/02/06 00:55:56 chris Exp $

GENASSYM_EXTRAS+=	${ARM}/footbridge/genassym.cf
LOOSE_PROTOTYPES=	NO

.if (${OBJECT_FMT} == "ELF")
# Under ELF we need to do a whole load of stuff extra to get that kernel.

LOADADDRESS=0xF0000020

SYSTEM_LD_TAIL_EXTRA=; \
	${DBSYM} $@ || true; \
	$S/arch/${MACHINE}/conf/elf2aout.sh
.endif	
