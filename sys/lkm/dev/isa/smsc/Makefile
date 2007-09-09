# $NetBSD: Makefile,v 1.1 2007/09/09 05:18:20 xtraeme Exp $

S=		${.CURDIR}/../../../..
.PATH:		$S/dev/isa

KMOD= 		smsc_isa
SRCS= 		smsc.c lkm_isa.c
NOMAN=		# yes

LKM_ISA_IO_PORT=	0x02e
LKM_ISA_DRVNAME=	smsc
LKM_ISA_DRVNAME_CA=	smsc

#CPPFLAGS+=	-DMULTIPROCESSOR -DDEBUG -DDIAGNOSTIC -DLOCKDEBUG
CLEANFILES+=	*~ ${KMOD}
WARNS=		2

.include <bsd.kmod.mk>
