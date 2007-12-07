# $NetBSD: Makefile,v 1.1.8.2 2007/12/07 17:34:00 yamt Exp $

S=		${.CURDIR}/../../../..
.PATH:		$S/dev/isa

KMOD= 		itesio
SRCS+= 		lkm_isa.c itesio_isa.c
NOMAN=		# yes

LKM_ISA_IO_PORT=	0x2e
LKM_ISA_DRVNAME=	itesio

CLEANFILES+=	*~ ${KMOD}
WARNS=		2

.include <bsd.kmod.mk>
