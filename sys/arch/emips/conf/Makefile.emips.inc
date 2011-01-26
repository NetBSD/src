#	$NetBSD: Makefile.emips.inc,v 1.1 2011/01/26 01:18:50 pooka Exp $

SYSTEM_LD_TAIL_EXTRA=; \
		echo ${ELF2ECOFF} $@ $@.ecoff; ${ELF2ECOFF} $@ $@.ecoff; \
		chmod 755 $@.ecoff
