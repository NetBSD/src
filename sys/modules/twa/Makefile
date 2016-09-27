#	$NetBSD: Makefile,v 1.1 2016/09/27 03:53:09 pgoyette Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/pci

KMOD=	twa
IOCONF=	twa.ioconf

SRCS+=	twa.c

CPPFLAGS+=	-DDIAGNOSTIC

.include <bsd.kmodule.mk>
