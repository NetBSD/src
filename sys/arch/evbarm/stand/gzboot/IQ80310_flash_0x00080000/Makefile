#	$NetBSD: Makefile,v 1.3.150.1 2012/12/06 17:14:23 matt Exp $

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
