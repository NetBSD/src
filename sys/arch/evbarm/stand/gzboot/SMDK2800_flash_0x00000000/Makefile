#	$NetBSD: Makefile,v 1.3 2004/04/22 22:41:42 thorpej Exp $

S=	${.CURDIR}/../../../../..

PLATFORM= SMDK2800
RELOC= 0x00000000
MAXIMAGESIZE= 0		# concatenate
LOADADDR= 0x08200000

CPPFLAGS+= -DCPU_S3C2800
CPPFLAGS+= -DCONSPEED=115200
CPPFLAGS+= -DCONADDR=0x10170000UL
# CPPFLAGS+= -DCONADDR=0x10180000UL
CPPFLAGS+= -DIO_INIT_HOOK=smdk2800_io_init -DRAM_INIT_HOOK=smdk2800_ram_init -DXTAL_CLK=6000000
CPUFLAGS= -march=armv4

LDSCRIPT= ${.CURDIR}/ldscript

SRCS+=	smdk2800.c sscom.c
SRCS+=	smdk2800_io_init.c s3c2800_vector.S  smdk2800_ram_init.S

.include "${S}/arch/evbarm/stand/gzboot/Makefile.gzboot"
