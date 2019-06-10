#	$NetBSD: Makefile.sgimips.inc,v 1.12.56.1 2019/06/10 22:06:43 christos Exp $

WANT_ECOFF?=no

GENASSYM_EXTRAS+=	${S}/dev/arcbios/genassym.cf

.if ${WANT_ECOFF} == "yes"
SYSTEM_LD_TAIL_EXTRA=; \
		echo ${MIPS_ELF2ECOFF} $@ $@.ecoff; \
		${MIPS_ELF2ECOFF} $@ $@.ecoff; \
		chmod 755 $@.ecoff
.endif
