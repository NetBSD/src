#	$NetBSD: Makefile,v 1.1.2.2 2016/10/05 20:56:08 skrll Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/pci

KMOD=	twe
IOCONF=	twe.ioconf

SRCS+=	twe.c

CPPFLAGS+=	-DDIAGNOSTIC

.include <bsd.kmodule.mk>
