#	$NetBSD: Makefile.pmax.inc,v 1.2.8.3 2002/04/01 07:41:53 nathanw Exp $

.if ${USETOOLS} == "yes"
ELF2ECOFF?=	${TOOLDIR}/bin/nbmips-elf2ecoff
.else
ELF2ECOFF?=	elf2ecoff
.endif

SYSTEM_LD_TAIL_EXTRA=; \
		echo ${ELF2ECOFF} $@ $@.ecoff; ${ELF2ECOFF} $@ $@.ecoff; \
		chmod 755 $@.ecoff
