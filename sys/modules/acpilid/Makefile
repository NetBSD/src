# $NetBSD: Makefile,v 1.1.4.1 2011/03/05 20:55:36 rmind Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/acpi

KMOD=	acpilid
IOCONF=	acpilid.ioconf
SRCS=	acpi_lid.c

WARNS=	4

.include <bsd.kmodule.mk>
