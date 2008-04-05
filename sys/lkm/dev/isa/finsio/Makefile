# $NetBSD: Makefile,v 1.2 2008/04/05 19:03:57 xtraeme Exp $

S=		${.CURDIR}/../../../..
.PATH:		$S/dev/isa

KMOD= 		finsio
SRCS+= 		lkm_isa.c finsio_isa.c

LKM_ISA_IO_PORT=	0x4e
LKM_ISA_DRVNAME=	finsio

.include <bsd.kmod.mk>
