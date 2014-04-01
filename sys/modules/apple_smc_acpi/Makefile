# $NetBSD: Makefile,v 1.1 2014/04/01 17:48:39 riastradh Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/acpi

KMOD=	apple_smc_acpi
IOCONF=	apple_smc_acpi.ioconf
SRCS=	apple_smc_acpi.c

WARNS=	4

CPPFLAGS+=	-DDIAGNOSTIC

.include <bsd.kmodule.mk>
