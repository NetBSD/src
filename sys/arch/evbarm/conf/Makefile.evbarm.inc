#	$NetBSD: Makefile.evbarm.inc,v 1.15 2003/01/03 02:02:14 thorpej Exp $

SYSTEM_LD_TAIL_EXTRA=; \
	echo "${DBSYM} $@ || true"; \
	${DBSYM} $@ || true

.if (${BOARDTYPE} == "integrator")
GENASSYM_EXTRAS+=	${THISARM}/ifpga/genassym.cf
SYSTEM_FIRST_OBJ=	intmmu.o
SYSTEM_FIRST_SFILE=	${THISARM}/integrator/intmmu.S

KERNEL_BASE_PHYS=0x00200000
KERNEL_BASE_VIRT=0xc0200000

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

KERNEL_BASE_PHYS=0xc0200100
KERNEL_BASE_VIRT=0xc0200100

SYSTEM_LD_TAIL_EXTRA=
.endif

.if (${BOARDTYPE} == "iq80310")
SYSTEM_FIRST_OBJ=	iq80310_start.o
SYSTEM_FIRST_SFILE=	${THISARM}/iq80310/iq80310_start.S

KERNEL_BASE_PHYS=0xa0200000
KERNEL_BASE_VIRT=0xc0200000

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

KERNEL_BASE_PHYS=0x00200000
KERNEL_BASE_VIRT=0xc0200000

SYSTEM_LD_TAIL_EXTRA+=; \
	echo ${OBJCOPY} -S -O srec $@ $@.srec; \
	${OBJCOPY} -S -O srec $@ $@.srec; \
	echo ${OBJCOPY} -S -O binary $@ $@.bin; \
	${OBJCOPY} -S -O binary $@ $@.bin

EXTRA_KERNELS+= ${KERNELS:@.KERNEL.@${.KERNEL.}.srec@}
EXTRA_KERNELS+= ${KERNELS:@.KERNEL.@${.KERNEL.}.bin@}
.endif

.if (${BOARDTYPE} == "lubbock" || ${BOARDTYPE} == "g4250ebx")
SYSTEM_FIRST_OBJ=	${BOARDTYPE}_start.o
SYSTEM_FIRST_SFILE=	${THISARM}/${BOARDTYPE}/${BOARDTYPE}_start.S

KERNEL_BASE_PHYS=0xa0200000
KERNEL_BASE_VIRT=0xc0200000

#GENASSYM_EXTRAS+=	${THISARM}/${BOARDTYPE}/genassym.cf
SYSTEM_LD_TAIL_EXTRA+=; \
	echo ${OBJCOPY} -S -O binary $@ $@.bin; \
	${OBJCOPY} -S -O binary $@ $@.bin; \
	echo gzip \< $@.bin \> $@.bin.gz; \
	gzip < $@.bin > $@.bin.gz

EXTRA_KERNELS+= ${KERNELS:@.KERNEL.@${.KERNEL.}.bin@}
EXTRA_KERNELS+= ${KERNELS:@.KERNEL.@${.KERNEL.}.bin.gz@}
.endif

.if (${BOARDTYPE} == "smdk2800")
SYSTEM_FIRST_OBJ=	${BOARDTYPE}_start.o
SYSTEM_FIRST_SFILE=	${THISARM}/smdk2xx0/${BOARDTYPE}_start.S

KERNEL_BASE_PHYS=0x08200000
KERNEL_BASE_VIRT=0xc0200000

#GENASSYM_EXTRAS+=	${THISARM}/${BOARDTYPE}/genassym.cf
SYSTEM_LD_TAIL_EXTRA+=; \
	echo ${OBJCOPY} -S -O srec $@ $@.srec; \
	${OBJCOPY} -S -O srec $@ $@.srec; \
	echo ${OBJCOPY} -S -O binary $@ $@.bin; \
	${OBJCOPY} -S -O binary $@ $@.bin;

EXTRA_KERNELS+= ${KERNELS:@.KERNEL.@${.KERNEL.}.srec@}
EXTRA_KERNELS+= ${KERNELS:@.KERNEL.@${.KERNEL.}.bin@}
.endif

.if defined(KERNEL_BASE_PHYS)

LINKFLAGS=	-T ldscript

netbsd: ldscript             # XXX
EXTRA_CLEAN+= ldscript tmp

# generate ldscript from common template 
ldscript: ${THISARM}/conf/ldscript.evbarm ${THISARM}/conf/Makefile.evbarm.inc
	echo ${KERNELS}
	sed -e 's/@KERNEL_BASE_PHYS@/${KERNEL_BASE_PHYS}/' \
	    -e 's/@KERNEL_BASE_VIRT@/${KERNEL_BASE_VIRT}/' \
	    ${THISARM}/conf/ldscript.evbarm > tmp && mv tmp $@

.endif	# KERNEL_BASE_PHYS
