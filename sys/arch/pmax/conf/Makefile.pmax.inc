#	$NetBSD: Makefile.pmax.inc,v 1.2.8.2 2002/01/08 00:27:04 nathanw Exp $
ELF2ECOFF?=	elf2ecoff

SYSTEM_LD_TAIL_EXTRA=; \
		echo ${ELF2ECOFF} $@ $@.ecoff; ${ELF2ECOFF} $@ $@.ecoff; \
		chmod 755 $@.ecoff
