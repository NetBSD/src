#	$NetBSD: Makefile.sgimips.inc,v 1.11.70.1 2011/03/05 20:51:52 rmind Exp $

WANT_ECOFF?=no

GENASSYM_EXTRAS+=	${S}/dev/arcbios/genassym.cf

.if ${WANT_ECOFF} == "yes"
SYSTEM_LD_TAIL_EXTRA=; \
		echo ${ELF2ECOFF} $@ $@.ecoff; ${ELF2ECOFF} $@ $@.ecoff; \
		chmod 755 $@.ecoff
.endif
