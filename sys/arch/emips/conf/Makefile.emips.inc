#	$NetBSD: Makefile.emips.inc,v 1.1.4.2 2011/03/05 20:49:53 rmind Exp $

SYSTEM_LD_TAIL_EXTRA=; \
		echo ${ELF2ECOFF} $@ $@.ecoff; ${ELF2ECOFF} $@ $@.ecoff; \
		chmod 755 $@.ecoff
