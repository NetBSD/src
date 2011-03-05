#	$NetBSD: Makefile,v 1.3.2.2 2011/03/05 20:55:36 rmind Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/acpi

KMOD=	acpitz
IOCONF=	acpitz.ioconf
SRCS=	acpi_tz.c

WARNS=	4

.include <bsd.kmodule.mk>
