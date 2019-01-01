#	$NetBSD: Makefile.pmax.inc,v 1.7 2019/01/01 19:41:04 christos Exp $

.if ${MACHINE_ARCH} == "mipsel"
SYSTEM_LD_TAIL_EXTRA=; \
		echo ${MIPS_ELF2ECOFF} $@ $@.ecoff; \
		${MIPS_ELF2ECOFF} $@ $@.ecoff; \
		chmod 755 $@.ecoff
.endif
