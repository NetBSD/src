#	$NetBSD: Makefile,v 1.3 2003/04/29 05:38:52 thorpej Exp $

S=	${.CURDIR}/../../../../..

PLATFORM= IQ80310
RELOC= 0x00080000
MAXIMAGESIZE= 0		# concatenate
LOADADDR= 0xa0200000

CPPFLAGS+= -DCONSPEED=115200
CPPFLAGS+= -DCONADDR=0xfe810000UL

LDSCRIPT= ${.CURDIR}/ldscript

SRCS+=	iq80310.c ns16550.c i80312_mem.c

.include "${S}/arch/evbarm/stand/gzboot/Makefile.gzboot"
