#	$NetBSD: Makefile,v 1.3 2012/12/12 15:08:44 matt Exp $

S=	${.CURDIR}/../../../../..

PLATFORM= ADI_BRH
RELOC= 0x00140000
MAXIMAGESIZE= 0		# concatenate
LOADADDR= 0xc0200000

CPPFLAGS+= -DBECC_SUPPORT_V7
CPPFLAGS+= -DCONSPEED=57600
CPPFLAGS+= -DCONADDR=0x03000000UL
CPPFLAGS+= -DNS16550_FREQ=33300000
CPUFLAGS= -mcpu=xscale

LDSCRIPT= ${.CURDIR}/ldscript

SRCS+=	adi_brh.c ns16550.c becc_mem.c

.include "${S}/arch/evbarm/stand/gzboot/Makefile.gzboot"
