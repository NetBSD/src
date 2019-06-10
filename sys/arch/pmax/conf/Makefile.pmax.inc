#	$NetBSD: Makefile.pmax.inc,v 1.6.64.1 2019/06/10 22:06:37 christos Exp $

.if ${MACHINE_ARCH} == "mipsel"
SYSTEM_LD_TAIL_EXTRA=; \
		echo ${MIPS_ELF2ECOFF} $@ $@.ecoff; \
		${MIPS_ELF2ECOFF} $@ $@.ecoff; \
		chmod 755 $@.ecoff
.endif
