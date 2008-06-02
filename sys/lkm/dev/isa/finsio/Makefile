# $NetBSD: Makefile,v 1.2.8.2 2008/06/02 13:24:17 mjf Exp $

S=		${.CURDIR}/../../../..
.PATH:		$S/dev/isa

KMOD= 		finsio
SRCS+= 		lkm_isa.c finsio_isa.c

LKM_ISA_IO_PORT=	0x4e
LKM_ISA_DRVNAME=	finsio

.include <bsd.kmod.mk>
