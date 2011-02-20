#	$NetBSD: Makefile.sgimips.inc,v 1.12 2011/02/20 07:59:49 matt Exp $

WANT_ECOFF?=no

GENASSYM_EXTRAS+=	${S}/dev/arcbios/genassym.cf

.if ${WANT_ECOFF} == "yes"
SYSTEM_LD_TAIL_EXTRA=; \
		echo ${ELF2ECOFF} $@ $@.ecoff; ${ELF2ECOFF} $@ $@.ecoff; \
		chmod 755 $@.ecoff
.endif
