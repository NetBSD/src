#	$NetBSD: Makefile.pmax.inc,v 1.2.2.1 2002/01/10 19:47:46 thorpej Exp $
ELF2ECOFF?=	elf2ecoff

SYSTEM_LD_TAIL_EXTRA=; \
		echo ${ELF2ECOFF} $@ $@.ecoff; ${ELF2ECOFF} $@ $@.ecoff; \
		chmod 755 $@.ecoff
