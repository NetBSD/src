#	$NetBSD: Makefile.sgimips.inc,v 1.7 2002/05/21 21:01:34 rafal Exp $
SYSTEM_LD_TAIL_EXTRA=; \
		echo ${ELF2ECOFF} $@ $@.ecoff; ${ELF2ECOFF} $@ $@.ecoff; \
		chmod 755 $@.ecoff
