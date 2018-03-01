#	$NetBSD: Makefile,v 1.2 2018/03/01 04:53:05 pgoyette Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev
.PATH:	${S}/arch/x86/pci
.PATH:	${S}/arch/x86/pci/imcsmb

KMOD=	imc
IOCONF= imc.ioconf

SRCS+=	imc.c

.include <bsd.kmodule.mk>
