# $NetBSD: Makefile,v 1.3 2008/04/05 19:03:57 xtraeme Exp $

S=		${.CURDIR}/../../../..
.PATH:		$S/dev/isa

KMOD= 		ug
SRCS+= 		lkm_isa.c ug_isa.c

LKM_ISA_IO_PORT=	0xe0
LKM_ISA_DRVNAME=	ug
LKM_ISA_DRVNAME_CA=	ug_isa

.include <bsd.kmod.mk>
