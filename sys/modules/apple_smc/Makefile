# $NetBSD: Makefile,v 1.1 2014/04/01 17:47:36 riastradh Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/ic
.PATH:	${S}/dev/acpi

KMOD=	apple_smc
IOCONF=	apple_smc.ioconf
SRCS=	apple_smc.c apple_smc_acpi.c apple_smc_fan.c apple_smc_temp.c

WARNS=	4

# XXX What's the right thing here?
CPPFLAGS+=	-DDIAGNOSTIC

.include <bsd.kmodule.mk>
