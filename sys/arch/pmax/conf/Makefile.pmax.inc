#	$NetBSD: Makefile.pmax.inc,v 1.5 2002/04/12 05:11:17 lukem Exp $

SYSTEM_LD_TAIL_EXTRA=; \
		echo ${ELF2ECOFF} $@ $@.ecoff; ${ELF2ECOFF} $@ $@.ecoff; \
		chmod 755 $@.ecoff
