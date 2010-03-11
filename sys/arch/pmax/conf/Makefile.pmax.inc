#	$NetBSD: Makefile.pmax.inc,v 1.5.114.1 2010/03/11 15:02:48 yamt Exp $

.if ${MACHINE_ARCH} == "mipsel"
SYSTEM_LD_TAIL_EXTRA=; \
		echo ${ELF2ECOFF} $@ $@.ecoff; ${ELF2ECOFF} $@ $@.ecoff; \
		chmod 755 $@.ecoff
.endif
