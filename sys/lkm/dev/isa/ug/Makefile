# $NetBSD: Makefile,v 1.2.28.1 2008/06/02 13:24:17 mjf Exp $

S=		${.CURDIR}/../../../..
.PATH:		$S/dev/isa

KMOD= 		ug
SRCS+= 		lkm_isa.c ug_isa.c

LKM_ISA_IO_PORT=	0xe0
LKM_ISA_DRVNAME=	ug
LKM_ISA_DRVNAME_CA=	ug_isa

.include <bsd.kmod.mk>
