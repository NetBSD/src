#	$NetBSD: Makefile.mipsco.inc,v 1.5 2002/03/19 00:58:17 atatat Exp $

# build ECOFF image that can be booted directly from firmware
LD_ECOFF=	${LD} -N -Ttext ${TEXTADDR} -e start ${GP} \
		-T ${THISMIPS}/conf/kern.ldscript -x --oformat ecoff-bigmips

SYSTEM_LD_TAIL_EXTRA=; \
		echo ${LD_ECOFF} -o $@.ecoff '$${SYSTEM_OBJ}' vers.o ; \
		${LD_ECOFF} -o $@.ecoff ${SYSTEM_OBJ} vers.o ${SWAP_OBJ$@} ; \
		${SIZE} $@.ecoff; chmod 755 $@.ecoff

EXTRA_KERNELS+= ${KERNELS:@.KERNEL.@${.KERNEL.}.ecoff@}
