#	$NetBSD: Makefile.pmax.inc,v 1.3 2001/12/09 05:00:49 atatat Exp $
ELF2ECOFF?=	elf2ecoff

SYSTEM_LD_TAIL_EXTRA=; \
		echo ${ELF2ECOFF} $@ $@.ecoff; ${ELF2ECOFF} $@ $@.ecoff; \
		chmod 755 $@.ecoff
