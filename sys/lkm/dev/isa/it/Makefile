# $NetBSD: Makefile,v 1.2.10.2 2007/10/27 11:35:47 yamt Exp $

S=		${.CURDIR}/../../../..
.PATH:		$S/dev/isa

KMOD= 		it_isa
SRCS+= 		lkm_isa.c it.c
NOMAN=		# yes

LKM_ISA_IO_PORT=	0x290	# Other ports: 0xc00, 0xd00
LKM_ISA_DRVNAME=	it
LKM_ISA_DRVNAME_CA=	it_isa

CLEANFILES+=	*~ ${KMOD}
WARNS=		2

.include <bsd.kmod.mk>
