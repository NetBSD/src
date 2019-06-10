# $NetBSD: Makefile,v 1.1.36.1 2019/06/10 22:09:09 christos Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/acpi

KMOD=	apple_smc_acpi
IOCONF=	apple_smc_acpi.ioconf
SRCS=	apple_smc_acpi.c

CPPFLAGS+=	-DDIAGNOSTIC

.include <bsd.kmodule.mk>
