#	$NetBSD: Makefile,v 1.2.22.2 2013/01/16 05:32:56 yamt Exp $

S=	${.CURDIR}/../../../../..

PLATFORM= GEMINI
RELOC= 0x01600000
MAXIMAGESIZE= 0		# concatenate
LOADADDR= 0x00200000

CPPFLAGS+= -DCONSPEED=19200
CPPFLAGS+= -DCONADDR=0x42000000UL
CPPFLAGS+= -DNS16550_AX4
CPPFLAGS+= -DNS16550_FREQ=48000000

CPUFLAGS= -march=armv4

LDSCRIPT= ${.CURDIR}/ldscript

# SRCS+=	gemini.c ns16550.c gemini_mem.c
SRCS+=	gemini.c nullcom.c gemini_mem.c

.include "${S}/arch/evbarm/stand/gzboot/Makefile.gzboot"
