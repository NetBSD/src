#	$NetBSD: Makefile,v 1.1 2002/03/28 22:03:01 thorpej Exp $

.include "../../../../Makefile.inc"

S=	${KERNSRCDIR}

PLATFORM= IQ80310
RELOC=	0x00080000
MAXIMAGESIZE= 1048576	# 1M
LOADADDR= 0xa0200000

CPPFLAGS+= -DCONSPEED=115200
CPPFLAGS+= -DCONADDR=0xfe810000UL

LDSCRIPT= ${S}/arch/evbarm/stand/gzboot/IQ80310_flash_0x80000/ldscript

SRCS+=	ns16550.c i80312_mem.c

.include "${S}/arch/evbarm/stand/gzboot/Makefile.gzboot"
