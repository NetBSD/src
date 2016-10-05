#	$NetBSD: Makefile,v 1.1.2.2 2016/10/05 20:56:07 skrll Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/pci

KMOD=	twa
IOCONF=	twa.ioconf

SRCS+=	twa.c

CPPFLAGS+=	-DDIAGNOSTIC

.include <bsd.kmodule.mk>
