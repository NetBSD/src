#	$NetBSD: Makefile,v 1.1.2.2 2016/10/05 20:56:06 skrll Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/ic

KMOD=	ld_cac
IOCONF=	ld_cac.ioconf

SRCS+=	ld_cac.c

CPPFLAGS+=	-DNBIO=1

.include <bsd.kmodule.mk>
