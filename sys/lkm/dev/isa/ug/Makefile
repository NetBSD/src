# $NetBSD: Makefile,v 1.2.2.2 2007/09/10 10:56:11 skrll Exp $

S=		${.CURDIR}/../../../..
.PATH:		$S/dev/isa

KMOD= 		ug
SRCS+= 		lkm_isa.c ug_isa.c
NOMAN=		# yes

LKM_ISA_IO_PORT=	0xe0
LKM_ISA_DRVNAME=	ug
LKM_ISA_DRVNAME_CA=	ug_isa

CLEANFILES+=	*~ ${KMOD}
WARNS=		2

.include <bsd.kmod.mk>
