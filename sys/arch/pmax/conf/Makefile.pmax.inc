#	$NetBSD: Makefile.pmax.inc,v 1.2.8.4 2002/04/17 00:04:09 nathanw Exp $

SYSTEM_LD_TAIL_EXTRA=; \
		echo ${ELF2ECOFF} $@ $@.ecoff; ${ELF2ECOFF} $@ $@.ecoff; \
		chmod 755 $@.ecoff
