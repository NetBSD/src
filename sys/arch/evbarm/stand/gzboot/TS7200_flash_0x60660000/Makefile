#	$NetBSD: Makefile,v 1.2.112.1 2013/01/16 05:32:57 yamt Exp $

S=	${.CURDIR}/../../../../..

PLATFORM= TS7200
RELOC= 0x60660000
MAXIMAGESIZE= 0		# concatenate
LOADADDR= 0x00200000

CPPFLAGS+= -DCONSPEED=115200
CPPFLAGS+= -DCONADDR=0x808c0000UL

CPUFLAGS= -mcpu=arm9tdmi

LDSCRIPT= ${.CURDIR}/ldscript

SRCS+=	ts7200.c epcom.c

.include "${S}/arch/evbarm/stand/gzboot/Makefile.gzboot"
