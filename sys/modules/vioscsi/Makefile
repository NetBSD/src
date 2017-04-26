#	$NetBSD: Makefile,v 1.1.4.2 2017/04/26 02:53:28 pgoyette Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/pci

KMOD=	vioscsi
IOCONF=	vioscsi.ioconf

SRCS+=	vioscsi.c

DBG+=	-DDEBUG -DDIAGNOSTIC -DLOCKDEBUG

.include <bsd.kmodule.mk>
