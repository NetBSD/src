#	$NetBSD: Makefile,v 1.2 2003/04/08 21:41:18 thorpej Exp $

NOMAN=	# defined

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

.include <bsd.kernobj.mk>

S=	${KERNSRCDIR}

PLATFORM= ADI_BRH
RELOC= 0x00140000
MAXIMAGESIZE= 1572864	# 1.5M
LOADADDR= 0xc0200000

CPPFLAGS+= -DBECC_SUPPORT_V7
CPPFLAGS+= -DCONSPEED=57600
CPPFLAGS+= -DCONADDR=0x03000000UL
CPPFLAGS+= -DNS16550_FREQ=33300000

LDSCRIPT= ${S}/arch/evbarm/stand/gzboot/ADI_BRH_flash_0x00140000/ldscript

SRCS+=	adi_brh.c ns16550.c becc_mem.c

.include "${S}/arch/evbarm/stand/gzboot/Makefile.gzboot"
