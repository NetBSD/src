# $NetBSD: Makefile,v 1.1.2.3 2010/08/11 22:54:49 yamt Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/acpi

KMOD=	acpibat
IOCONF=	ACPIBAT.ioconf
SRCS=	acpi_bat.c

.include <bsd.kmodule.mk>
