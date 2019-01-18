#	$NetBSD: Makefile.pmax.inc,v 1.6.62.1 2019/01/18 08:50:20 pgoyette Exp $

.if ${MACHINE_ARCH} == "mipsel"
SYSTEM_LD_TAIL_EXTRA=; \
		echo ${MIPS_ELF2ECOFF} $@ $@.ecoff; \
		${MIPS_ELF2ECOFF} $@ $@.ecoff; \
		chmod 755 $@.ecoff
.endif
