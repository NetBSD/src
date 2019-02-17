# $NetBSD: Makefile,v 1.2 2019/02/17 04:05:43 rin Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/acpi

KMOD=	apple_smc_acpi
IOCONF=	apple_smc_acpi.ioconf
SRCS=	apple_smc_acpi.c

CPPFLAGS+=	-DDIAGNOSTIC

.include <bsd.kmodule.mk>
