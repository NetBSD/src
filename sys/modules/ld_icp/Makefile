#	$NetBSD: Makefile,v 1.1.18.2 2017/12/03 11:38:57 jdolecek Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/ic

KMOD=	ld_icp
IOCONF=	ld_icp.ioconf

SRCS+=	ld_icp.c

CPPFLAGS+=	-DNBIO=1

.include <bsd.kmodule.mk>
