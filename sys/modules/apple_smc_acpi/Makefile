# $NetBSD: Makefile,v 1.1.4.2 2014/05/18 17:46:10 rmind Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/acpi

KMOD=	apple_smc_acpi
IOCONF=	apple_smc_acpi.ioconf
SRCS=	apple_smc_acpi.c

WARNS=	4

CPPFLAGS+=	-DDIAGNOSTIC

.include <bsd.kmodule.mk>
