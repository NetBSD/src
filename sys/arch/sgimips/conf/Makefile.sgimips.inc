#	$NetBSD: Makefile.sgimips.inc,v 1.6.8.1 2002/05/30 15:35:37 gehenna Exp $
SYSTEM_LD_TAIL_EXTRA=; \
		echo ${ELF2ECOFF} $@ $@.ecoff; ${ELF2ECOFF} $@ $@.ecoff; \
		chmod 755 $@.ecoff
