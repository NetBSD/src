# $NetBSD: Makefile,v 1.1 2007/09/11 21:46:53 xtraeme Exp $

S=		${.CURDIR}/../../../..
.PATH:		$S/dev/isa $S/dev/ic

KMOD= 		aps_isa
SRCS= 		aps.c lkm_isa.c
NOMAN=		# yes

LKM_ISA_IO_PORT=	0x1600
LKM_ISA_DRVNAME=	aps

CLEANFILES+=	*~ ${KMOD}
WARNS=		2

CPPFLAGS+=	-DAPSDEBUG -DMULTIPROCESSOR -DDIAGNOSTIC -DDEBUG -DLOCKDEBUG

.include <bsd.kmod.mk>
