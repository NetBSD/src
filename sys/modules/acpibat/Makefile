# $NetBSD: Makefile,v 1.1.4.1 2010/05/30 05:18:00 rmind Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/acpi

KMOD=	acpibat
IOCONF=	ACPIBAT.ioconf
SRCS=	acpi_bat.c

.include <bsd.kmodule.mk>
