#	$NetBSD: Makefile,v 1.1 2018/02/25 08:20:28 pgoyette Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev
.PATH:	${S}/dev/pci
.PATH:	${S}/dev/imcsmb

KMOD=	imc
IOCONF= imc.ioconf

SRCS+=	imc.c

.include <bsd.kmodule.mk>
