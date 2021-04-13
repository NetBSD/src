# $NetBSD: Makefile,v 1.3 2021/04/13 06:25:49 mrg Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/acpi

KMOD=	hpacel
IOCONF=	hpacel.ioconf
SRCS=	hpacel_acpi.c

COPTS.hpacel_acpi.c+=       ${GCC_NO_MAYBE_UNINITIALIZED}

.include <bsd.kmodule.mk>
