# $NetBSD: Makefile,v 1.2.2.2 2007/09/10 10:56:08 skrll Exp $

S=		${.CURDIR}/../../../..
.PATH:		$S/dev/isa $S/dev/ic

KMOD= 		lm
SRCS= 		lm_isa.c nslm7x.c lkm_isa.c
NOMAN=		# yes

LKM_ISA_IO_PORT=	0x290
LKM_ISA_DRVNAME=	lm
LKM_ISA_DRVNAME_CA=	lm_isa

CLEANFILES+=	*~ ${KMOD}
WARNS=		2

.include <bsd.kmod.mk>
