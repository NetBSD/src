#	$NetBSD: Makefile.emips.inc,v 1.1.2.2 2011/02/08 18:05:05 bouyer Exp $

SYSTEM_LD_TAIL_EXTRA=; \
		echo ${ELF2ECOFF} $@ $@.ecoff; ${ELF2ECOFF} $@ $@.ecoff; \
		chmod 755 $@.ecoff
