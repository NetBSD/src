#	$NetBSD: Makefile.evbarm.inc,v 1.13 2002/07/15 17:13:30 ichiro Exp $

SYSTEM_LD_TAIL_EXTRA=; \
	echo "${DBSYM} $@ || true"; \
	${DBSYM} $@ || true

.if (${BOARDTYPE} == "integrator")
GENASSYM_EXTRAS+=	${THISARM}/ifpga/genassym.cf
SYSTEM_FIRST_OBJ=	intmmu.o
SYSTEM_FIRST_SFILE=	${THISARM}/integrator/intmmu.S

LINKFLAGS=	-T ${THISARM}/conf/ldscript.integrator

SYSTEM_LD_TAIL_EXTRA+=; \
	echo ${OBJCOPY} -S -O srec $@ $@.srec; \
	${OBJCOPY} -S -O srec $@ $@.srec; \
	echo ${OBJCOPY} -S -O binary $@ $@.bin; \
	${OBJCOPY} -S -O binary $@ $@.bin

EXTRA_KERNELS+= ${KERNELS:@.KERNEL.@${.KERNEL.}.srec@}
EXTRA_KERNELS+= ${KERNELS:@.KERNEL.@${.KERNEL.}.bin@}
.endif

.if (${BOARDTYPE} == "ixm1200")
SYSTEM_FIRST_OBJ=	ixm1200_start.o
SYSTEM_FIRST_SFILE=	${THISARM}/ixm1200/ixm1200_start.S

LINKFLAGS=	-T ${THISARM}/conf/ldscript.ixm1200

SYSTEM_LD_TAIL_EXTRA=
.endif

.if (${BOARDTYPE} == "iq80310")
SYSTEM_FIRST_OBJ=	iq80310_start.o
SYSTEM_FIRST_SFILE=	${THISARM}/iq80310/iq80310_start.S

LINKFLAGS=	-T ${THISARM}/conf/ldscript.iq80310

SYSTEM_LD_TAIL_EXTRA+=; \
	echo ${OBJCOPY} -S -O srec $@ $@.srec; \
	${OBJCOPY} -S -O srec $@ $@.srec; \
	echo ${OBJCOPY} -S -O binary $@ $@.bin; \
	${OBJCOPY} -S -O binary $@ $@.bin

EXTRA_KERNELS+= ${KERNELS:@.KERNEL.@${.KERNEL.}.srec@}
EXTRA_KERNELS+= ${KERNELS:@.KERNEL.@${.KERNEL.}.bin@}
.endif

.if (${BOARDTYPE} == "iq80321")
SYSTEM_FIRST_OBJ=	iq80321_start.o
SYSTEM_FIRST_SFILE=	${THISARM}/iq80321/iq80321_start.S

LINKFLAGS=	-T ${THISARM}/conf/ldscript.iq80321

SYSTEM_LD_TAIL_EXTRA+=; \
	echo ${OBJCOPY} -S -O srec $@ $@.srec; \
	${OBJCOPY} -S -O srec $@ $@.srec; \
	echo ${OBJCOPY} -S -O binary $@ $@.bin; \
	${OBJCOPY} -S -O binary $@ $@.bin

EXTRA_KERNELS+= ${KERNELS:@.KERNEL.@${.KERNEL.}.srec@}
EXTRA_KERNELS+= ${KERNELS:@.KERNEL.@${.KERNEL.}.bin@}
.endif
