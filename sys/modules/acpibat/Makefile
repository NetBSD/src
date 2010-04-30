# $NetBSD: Makefile,v 1.2.2.2 2010/04/30 14:44:16 uebayasi Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/acpi

KMOD=	acpibat
IOCONF=	ACPIBAT.ioconf
SRCS=	acpi_bat.c

.include <bsd.kmodule.mk>
