# $NetBSD: Makefile,v 1.4 2008/04/05 19:03:57 xtraeme Exp $

S=		${.CURDIR}/../../../..
.PATH:		$S/dev/isa

KMOD= 		itesio
SRCS+= 		lkm_isa.c itesio_isa.c

LKM_ISA_IO_PORT=	0x2e
LKM_ISA_DRVNAME=	itesio

.include <bsd.kmod.mk>
