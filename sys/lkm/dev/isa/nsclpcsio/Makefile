# $NetBSD: Makefile,v 1.3.26.1 2008/06/02 13:24:17 mjf Exp $

S=		${.CURDIR}/../../../..
.PATH:		$S/dev/isa

KMOD= 		nsclpcsio
SRCS= 		nsclpcsio_isa.c lkm_isa.c

LKM_ISA_DRVNAME=	nsclpcsio
LKM_ISA_DRVNAME_CA=	nsclpcsio_isa

.include <bsd.kmod.mk>
