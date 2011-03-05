# $NetBSD: Makefile,v 1.1.4.2 2011/03/05 20:55:34 rmind Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/acpi

KMOD=	acpibat
IOCONF=	acpibat.ioconf
SRCS=	acpi_bat.c

WARNS=	4

.include <bsd.kmodule.mk>
