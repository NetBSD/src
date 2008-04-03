# $NetBSD: Makefile,v 1.3 2008/04/03 22:54:29 xtraeme Exp $

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
