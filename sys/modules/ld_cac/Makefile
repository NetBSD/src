#	$NetBSD: Makefile,v 1.1.18.2 2017/12/03 11:38:57 jdolecek Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/ic

KMOD=	ld_cac
IOCONF=	ld_cac.ioconf

SRCS+=	ld_cac.c

CPPFLAGS+=	-DNBIO=1

.include <bsd.kmodule.mk>
