#	$NetBSD: Makefile.mipsco.inc,v 1.6.12.1 2005/02/06 07:45:25 jmc Exp $

CPPFLAGS+=	-I$S/dist/ipf

SYSTEM_LD_TAIL_EXTRA=; \
		echo ${ELF2ECOFF} $@ $@.ecoff; ${ELF2ECOFF} $@ $@.ecoff; \
		${SIZE} $@.ecoff; chmod 755 $@.ecoff

EXTRA_KERNELS+= ${KERNELS:@.KERNEL.@${.KERNEL.}.ecoff@}
