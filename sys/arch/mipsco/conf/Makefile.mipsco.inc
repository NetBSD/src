#	$NetBSD: Makefile.mipsco.inc,v 1.6.196.1 2019/06/10 22:06:31 christos Exp $

SYSTEM_LD_TAIL_EXTRA=; \
		echo ${MIPS_ELF2ECOFF} $@ $@.ecoff; \
		${MIPS_ELF2ECOFF} $@ $@.ecoff; \
		${SIZE} $@.ecoff; chmod 755 $@.ecoff

EXTRA_KERNELS+= ${KERNELS:@.KERNEL.@${.KERNEL.}.ecoff@}
