#	$NetBSD: Makefile.mipsco.inc,v 1.2.8.2 2002/01/11 23:38:40 nathanw Exp $

# build ECOFF image that can be booted directly from firmware
LD_ECOFF=	${LD} -N -Ttext ${TEXTADDR} -e start ${GP} \
		-T ${THISMIPS}/conf/kern.ldscript -x --oformat ecoff-bigmips

# note that this ecoff image does not get stripped in the case of DEBUG=-g
SYSTEM_LD_TAIL_EXTRA=; \
		echo ${LD_ECOFF} -o $@.ecoff '$${SYSTEM_OBJ}' vers.o ; \
		${LD_ECOFF} -o $@.ecoff ${SYSTEM_OBJ} vers.o ${SWAP_OBJ$@} ; \
		${SIZE} $@.ecoff; chmod 755 $@.ecoff

EXTRA_KERNELS+= ${KERNELS:@.KERNEL.@${.KERNEL.}.ecoff@}
