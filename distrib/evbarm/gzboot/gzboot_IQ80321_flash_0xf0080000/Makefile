#	$NetBSD: Makefile,v 1.1 2002/03/28 22:03:02 thorpej Exp $

.include "../../../../Makefile.inc"

S=	${KERNSRCDIR}

PLATFORM= IQ80321
RELOC=	0xf0080000
MAXIMAGESIZE= 1048576	# 1M
LOADADDR= 0xa0200000

CPPFLAGS+= -DCONSPEED=115200
CPPFLAGS+= -DCONADDR=0xfe800000UL

LDSCRIPT= ${S}/arch/evbarm/stand/gzboot/IQ80321_flash_0xf0080000/ldscript

SRCS+=	ns16550.c i80321_mem.c

.include "${S}/arch/evbarm/stand/gzboot/Makefile.gzboot"
