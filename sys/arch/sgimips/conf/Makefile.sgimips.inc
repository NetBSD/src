#	$NetBSD: Makefile.sgimips.inc,v 1.11.78.1 2011/03/05 15:10:01 bouyer Exp $

WANT_ECOFF?=no

GENASSYM_EXTRAS+=	${S}/dev/arcbios/genassym.cf

.if ${WANT_ECOFF} == "yes"
SYSTEM_LD_TAIL_EXTRA=; \
		echo ${ELF2ECOFF} $@ $@.ecoff; ${ELF2ECOFF} $@ $@.ecoff; \
		chmod 755 $@.ecoff
.endif
