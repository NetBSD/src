#	$NetBSD: Makefile,v 1.1 2003/07/30 18:54:18 bsh Exp $

S=	${.CURDIR}/../../../../..

PLATFORM= SMDK2800
RELOC= 0x00100000
MAXIMAGESIZE= 0		# concatenate
LOADADDR= 0x08200000

CPPFLAGS+= -DCONSPEED=115200
CPPFLAGS+= -DCONADDR=0x10170000UL
# CPPFLAGS+= -DCONADDR=0x10180000UL
CPPFLAGS+= -DXTAL_CLK=6000000
COPTS+= -march=armv4

LDSCRIPT= ${.CURDIR}/ldscript

SRCS+=	smdk2800.c sscom.c

.include "${S}/arch/evbarm/stand/gzboot/Makefile.gzboot"
