#	$NetBSD: Makefile,v 1.2 2002/04/26 15:50:29 lukem Exp $

NOMAN=	# defined

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/Makefile.inc"

.include <bsd.kernobj.mk>

S=	${KERNSRCDIR}

PLATFORM= TEAMASA_NPWR
RELOC=	0x00080000
MAXIMAGESIZE= 1048576	# 1M
LOADADDR= 0xa0200000

CPPFLAGS+= -DCONSPEED=115200
CPPFLAGS+= -DCONADDR=0xfe810000UL

LDSCRIPT= ${S}/arch/evbarm/stand/gzboot/IQ80310_flash_0x80000/ldscript

SRCS+=	iq80310.c ns16550.c i80312_mem.c

.include "${S}/arch/evbarm/stand/gzboot/Makefile.gzboot"
