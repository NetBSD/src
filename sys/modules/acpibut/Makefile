# $NetBSD: Makefile,v 1.4 2019/02/17 04:05:41 rin Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/acpi

KMOD=	acpibut
IOCONF=	acpibut.ioconf
SRCS=	acpi_button.c

.include <bsd.kmodule.mk>
