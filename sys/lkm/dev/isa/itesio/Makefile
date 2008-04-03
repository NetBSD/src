# $NetBSD: Makefile,v 1.2 2008/04/03 22:46:22 xtraeme Exp $

S=		${.CURDIR}/../../../..
.PATH:		$S/dev/isa

KMOD= 		finsio
SRCS+= 		lkm_isa.c finsio_isa.c
NOMAN=		# yes

LKM_ISA_IO_PORT=	0x4e
LKM_ISA_DRVNAME=	finsio

CLEANFILES+=	*~ ${KMOD}
WARNS=		2

.include <bsd.kmod.mk>
