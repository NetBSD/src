#	$NetBSD: Makefile.evbarm.inc,v 1.6.2.3 2002/02/11 20:07:41 jdolecek Exp $

.if (${BOARDTYPE} == "integrator")
GENASSYM_EXTRAS+=	${THISARM}/ifpga/genassym.cf
SYSTEM_FIRST_OBJ=	intmmu.o
SYSTEM_FIRST_SFILE=	${THISARM}/integrator/intmmu.S
.endif

SYSTEM_LD_TAIL_EXTRA=; \
	echo "${DBSYM} $@ || true"; \
	${DBSYM} $@ || true

.if (${BOARDTYPE} == "iq80310")
SYSTEM_LD_TAIL_EXTRA+=; \
	echo ${OBJCOPY} -S -O srec $@ $@.srec; \
	${OBJCOPY} -S -O srec $@ $@.srec
EXTRA_KERNELS+= ${KERNELS:@.KERNEL.@${.KERNEL.}.srec@}
.endif
