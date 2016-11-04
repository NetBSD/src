#	$NetBSD: Makefile,v 1.1.4.2 2016/11/04 14:49:20 pgoyette Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/pci

KMOD=	twa
IOCONF=	twa.ioconf

SRCS+=	twa.c

CPPFLAGS+=	-DDIAGNOSTIC

.include <bsd.kmodule.mk>
