#	$NetBSD: Makefile.sgimips.inc,v 1.4.4.3 2002/06/20 03:40:46 nathanw Exp $
SYSTEM_LD_TAIL_EXTRA=; \
		echo ${ELF2ECOFF} $@ $@.ecoff; ${ELF2ECOFF} $@ $@.ecoff; \
		chmod 755 $@.ecoff
