#	$NetBSD: Makefile,v 1.1 2004/12/24 10:32:40 joff Exp $

S=	${.CURDIR}/../../../../..

PLATFORM= TS7200
RELOC= 0x60660000
MAXIMAGESIZE= 0		# concatenate
LOADADDR= 0x00200000

CPPFLAGS+= -DCONSPEED=115200
CPPFLAGS+= -DCONADDR=0x808c0000UL

LDSCRIPT= ${.CURDIR}/ldscript

SRCS+=	ts7200.c epcom.c

.include "${S}/arch/evbarm/stand/gzboot/Makefile.gzboot"
