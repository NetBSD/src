# $NetBSD: Makefile,v 1.2.2.2 2007/09/10 10:56:09 skrll Exp $

S=		${.CURDIR}/../../../..
.PATH:		$S/dev/isa

KMOD= 		nsclpcsio
SRCS= 		nsclpcsio_isa.c lkm_isa.c
NOMAN=		# yes

LKM_ISA_IO_PORT=	0	# Driver tries to guess the I/O port
LKM_ISA_DRVNAME=	nsclpcsio
LKM_ISA_DRVNAME_CA=	nsclpcsio_isa

CLEANFILES+=	*~ ${KMOD}
WARNS=		2

.include <bsd.kmod.mk>
