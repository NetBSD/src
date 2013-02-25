#	$NetBSD: Makefile,v 1.3.152.1 2013/02/25 00:28:39 tls Exp $

S=	${.CURDIR}/../../../../..

PLATFORM= IQ80321
RELOC= 0xf0080000
MAXIMAGESIZE= 0		# concatenate
LOADADDR= 0xa0200000

CPPFLAGS+= -DCONSPEED=115200
CPPFLAGS+= -DCONADDR=0xfe800000UL

CPUFLAGS= -mcpu=xscale

LDSCRIPT= ${.CURDIR}/ldscript

SRCS+=	iq80321.c ns16550.c i80321_mem.c

.include "${S}/arch/evbarm/stand/gzboot/Makefile.gzboot"
