#	$NetBSD: Makefile,v 1.1.18.2 2017/12/03 11:39:00 jdolecek Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/pci

KMOD=	twa
IOCONF=	twa.ioconf

SRCS+=	twa.c

CPPFLAGS+=	-DDIAGNOSTIC

.include <bsd.kmodule.mk>
