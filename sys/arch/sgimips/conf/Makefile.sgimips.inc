#	$NetBSD: Makefile.sgimips.inc,v 1.2.2.2 2002/06/23 17:40:27 jdolecek Exp $
SYSTEM_LD_TAIL_EXTRA=; \
		echo ${ELF2ECOFF} $@ $@.ecoff; ${ELF2ECOFF} $@ $@.ecoff; \
		chmod 755 $@.ecoff
