#	$NetBSD: Makefile,v 1.1.2.3 2002/04/01 07:39:48 nathanw Exp $

S=	${.CURDIR}/../../../../..

PLATFORM= IQ80310
RELOC= 0x80000
MAXIMAGESIZE= 2097152	# 2M
LOADADDR= 0xa0200000

CPPFLAGS+= -DCONSPEED=115200
CPPFLAGS+= -DCONADDR=0xfe810000UL

LDSCRIPT= ${.CURDIR}/ldscript

SRCS+=	ns16550.c i80312_mem.c

.include "${S}/arch/evbarm/stand/gzboot/Makefile.gzboot"
