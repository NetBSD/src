# $NetBSD: Makefile,v 1.1.4.1 2011/02/17 12:00:46 bouyer Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/acpi

KMOD=	acpipmtr
IOCONF=	acpipmtr.ioconf
SRCS=	acpi_pmtr.c

WARNS=	4

.include <bsd.kmodule.mk>
