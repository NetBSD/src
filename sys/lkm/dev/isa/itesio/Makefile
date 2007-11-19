# $NetBSD: Makefile,v 1.1.2.2 2007/11/19 19:58:42 mjf Exp $

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
