#	$NetBSD: Makefile,v 1.2.8.2 2002/04/25 01:47:55 thorpej Exp $

S=	${.CURDIR}/../../../../..

PLATFORM= IQ80321
RELOC= 0xf0080000
MAXIMAGESIZE= 2097152	# 2M
LOADADDR= 0xa0200000

CPPFLAGS+= -DCONSPEED=115200
CPPFLAGS+= -DCONADDR=0xfe800000UL

LDSCRIPT= ${.CURDIR}/ldscript

SRCS+=	iq80321.c ns16550.c i80321_mem.c

.include "${S}/arch/evbarm/stand/gzboot/Makefile.gzboot"
