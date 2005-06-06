#	$NetBSD: mk.viper,v 1.1 2005/06/06 20:24:10 pooka Exp $

SYSTEM_FIRST_OBJ=	viper_start.o
SYSTEM_FIRST_SFILE=	${THISARM}/viper/viper_start.S

KERNEL_BASE_PHYS=0xa0200000
KERNEL_BASE_VIRT=0xc0200000

SYSTEM_LD_TAIL_EXTRA+=; \
	echo ${OBJCOPY} -S -O binary $@ $@.bin; \
	${OBJCOPY} -S -O binary $@ $@.bin; \
	echo gzip \< $@.bin \> $@.bin.gz; \
	gzip < $@.bin > $@.bin.gz

EXTRA_KERNELS+= ${KERNELS:@.KERNEL.@${.KERNEL.}.bin@}
EXTRA_KERNELS+= ${KERNELS:@.KERNEL.@${.KERNEL.}.bin.gz@}
