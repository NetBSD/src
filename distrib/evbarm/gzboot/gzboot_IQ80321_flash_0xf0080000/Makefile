#	$NetBSD: Makefile,v 1.7 2003/04/08 21:41:19 thorpej Exp $

NOMAN=	# defined

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

.include <bsd.kernobj.mk>

S=	${KERNSRCDIR}

PLATFORM= IQ80321
RELOC=	0xf0080000
MAXIMAGESIZE= 1572864	# 1.5M
LOADADDR= 0xa0200000

CPPFLAGS+= -DCONSPEED=115200
CPPFLAGS+= -DCONADDR=0xfe800000UL

LDSCRIPT= ${S}/arch/evbarm/stand/gzboot/IQ80321_flash_0xf0080000/ldscript

SRCS+=	iq80321.c ns16550.c i80321_mem.c

.include "${S}/arch/evbarm/stand/gzboot/Makefile.gzboot"
