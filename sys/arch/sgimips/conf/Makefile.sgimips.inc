#	$NetBSD: Makefile.sgimips.inc,v 1.7.8.1 2004/10/19 15:56:38 skrll Exp $

WANT_ECOFF?=no

.if ${WANT_ECOFF} == "yes"
SYSTEM_LD_TAIL_EXTRA=; \
		echo ${ELF2ECOFF} $@ $@.ecoff; ${ELF2ECOFF} $@ $@.ecoff; \
		chmod 755 $@.ecoff
.endif
