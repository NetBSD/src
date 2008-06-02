# $NetBSD: Makefile,v 1.1.22.1 2008/06/02 13:24:17 mjf Exp $

S=		${.CURDIR}/../../../..
.PATH:		$S/dev/isa

KMOD= 		itesio
SRCS+= 		lkm_isa.c itesio_isa.c

LKM_ISA_IO_PORT=	0x2e
LKM_ISA_DRVNAME=	itesio

.include <bsd.kmod.mk>
