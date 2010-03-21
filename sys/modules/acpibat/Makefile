# $NetBSD: Makefile,v 1.2 2010/03/21 07:09:56 pooka Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/acpi

KMOD=	acpibat
IOCONF=	ACPIBAT.ioconf
SRCS=	acpi_bat.c

.include <bsd.kmodule.mk>
