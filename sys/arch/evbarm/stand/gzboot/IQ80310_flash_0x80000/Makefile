#	$NetBSD: Makefile,v 1.1 2002/02/23 19:16:54 thorpej Exp $

S=	${.CURDIR}/../../../../..

PLATFORM= IQ80310
RELOC= 0x80000
MAXIMAGESIZE= 2097152	# 2M
LOADADDR= 0xa0200000

CPPFLAGS+= -DCONSPEED=115200

LDSCRIPT= ${.CURDIR}/ldscript

SRCS+=	iq80310_cons.c i80312_mem.c

.include "${S}/arch/evbarm/stand/gzboot/Makefile.gzboot"
