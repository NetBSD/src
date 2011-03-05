#	$NetBSD: Makefile,v 1.2.2.2 2011/03/05 20:55:44 rmind Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/acpi

KMOD=	hpqlb
IOCONF=	hpqlb.ioconf
SRCS=	hpqlb_acpi.c

WARNS=	4

.include <bsd.kmodule.mk>
