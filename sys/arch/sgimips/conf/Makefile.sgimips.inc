#	$NetBSD: Makefile.sgimips.inc,v 1.8 2004/10/01 07:05:25 sekiya Exp $

WANT_ECOFF?=no

.if ${WANT_ECOFF} == "yes"
SYSTEM_LD_TAIL_EXTRA=; \
		echo ${ELF2ECOFF} $@ $@.ecoff; ${ELF2ECOFF} $@ $@.ecoff; \
		chmod 755 $@.ecoff
.endif
