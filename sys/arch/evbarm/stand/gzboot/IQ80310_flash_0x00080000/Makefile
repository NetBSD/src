#	$NetBSD: Makefile,v 1.4 2012/12/12 15:08:44 matt Exp $

S=	${.CURDIR}/../../../../..

PLATFORM= IQ80310
RELOC= 0x00080000
MAXIMAGESIZE= 0		# concatenate
LOADADDR= 0xa0200000

CPPFLAGS+= -DCONSPEED=115200
CPPFLAGS+= -DCONADDR=0xfe810000UL

CPUFLAGS= -mcpu=xscale

LDSCRIPT= ${.CURDIR}/ldscript

SRCS+=	iq80310.c ns16550.c i80312_mem.c

.include "${S}/arch/evbarm/stand/gzboot/Makefile.gzboot"
