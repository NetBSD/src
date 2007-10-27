# $NetBSD: Makefile,v 1.3.8.2 2007/10/27 11:35:48 yamt Exp $

S=		${.CURDIR}/../../../..
.PATH:		$S/dev/isa

KMOD= 		nsclpcsio
SRCS= 		nsclpcsio_isa.c lkm_isa.c
NOMAN=		# yes

LKM_ISA_DRVNAME=	nsclpcsio
LKM_ISA_DRVNAME_CA=	nsclpcsio_isa

CLEANFILES+=	*~ ${KMOD}
WARNS=		2

.include <bsd.kmod.mk>
