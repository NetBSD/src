#	$NetBSD: Makefile.pmax.inc,v 1.4 2002/03/20 20:18:00 gmcgarry Exp $

.if ${USETOOLS} == "yes"
ELF2ECOFF?=	${TOOLDIR}/bin/nbmips-elf2ecoff
.else
ELF2ECOFF?=	elf2ecoff
.endif

SYSTEM_LD_TAIL_EXTRA=; \
		echo ${ELF2ECOFF} $@ $@.ecoff; ${ELF2ECOFF} $@ $@.ecoff; \
		chmod 755 $@.ecoff
