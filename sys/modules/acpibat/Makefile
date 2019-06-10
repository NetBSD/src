# $NetBSD: Makefile,v 1.4.56.1 2019/06/10 22:09:07 christos Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/acpi

KMOD=	acpibat
IOCONF=	acpibat.ioconf
SRCS=	acpi_bat.c

.include <bsd.kmodule.mk>
