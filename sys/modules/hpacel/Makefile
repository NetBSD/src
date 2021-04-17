# $NetBSD: Makefile,v 1.2.14.1 2021/04/17 17:26:21 thorpej Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/acpi

KMOD=	hpacel
IOCONF=	hpacel.ioconf
SRCS=	hpacel_acpi.c

COPTS.hpacel_acpi.c+=       ${GCC_NO_MAYBE_UNINITIALIZED}

.include <bsd.kmodule.mk>
