#	$NetBSD: Makefile.evbarm.inc,v 1.3.4.2 2002/01/08 00:24:11 nathanw Exp $

.if (${BOARDTYPE} == "integrator")
GENASSYM_EXTRAS+=	${THISARM}/ifpga/genassym.cf
SYSTEM_FIRST_OBJ=	intmmu.o
SYSTEM_FIRST_SFILE=	${THISARM}/integrator/intmmu.S
.endif

SYSTEM_LD_TAIL_EXTRA=; \
	echo "${DBSYM} $@ || true"; \
	${DBSYM} $@ || true

.if (${BOARDTYPE} == "iq80310")
GENASSYM_EXTRAS+=	${THISARM}/iq80310/genassym.cf
SYSTEM_LD_TAIL_EXTRA+=; \
	echo ${OBJCOPY} -S -O srec $@ $@.srec; \
	${OBJCOPY} -S -O srec $@ $@.srec
EXTRA_KERNELS+= ${KERNELS:@.KERNEL.@${.KERNEL.}.srec@}
.endif
