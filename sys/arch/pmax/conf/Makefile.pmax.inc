#	$NetBSD: Makefile.pmax.inc,v 1.6 2009/12/14 00:46:09 matt Exp $

.if ${MACHINE_ARCH} == "mipsel"
SYSTEM_LD_TAIL_EXTRA=; \
		echo ${ELF2ECOFF} $@ $@.ecoff; ${ELF2ECOFF} $@ $@.ecoff; \
		chmod 755 $@.ecoff
.endif
