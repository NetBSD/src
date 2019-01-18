#	$NetBSD: Makefile.sgimips.inc,v 1.12.54.1 2019/01/18 08:50:22 pgoyette Exp $

WANT_ECOFF?=no

GENASSYM_EXTRAS+=	${S}/dev/arcbios/genassym.cf

.if ${WANT_ECOFF} == "yes"
SYSTEM_LD_TAIL_EXTRA=; \
		echo ${MIPS_ELF2ECOFF} $@ $@.ecoff; \
		${MIPS_ELF2ECOFF} $@ $@.ecoff; \
		chmod 755 $@.ecoff
.endif
