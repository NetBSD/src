#	$NetBSD: Makefile.pmax.inc,v 1.5.132.1 2009/09/09 04:52:47 matt Exp $

.if ${MACHINE_ARCH} == "mipsel"
SYSTEM_LD_TAIL_EXTRA=; \
		echo ${ELF2ECOFF} $@ $@.ecoff; ${ELF2ECOFF} $@ $@.ecoff; \
		chmod 755 $@.ecoff
.endif
