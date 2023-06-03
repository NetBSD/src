# $NetBSD: Makefile,v 1.4 2023/06/03 09:09:14 lukem Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/acpi

KMOD=	hpacel
IOCONF=	hpacel.ioconf
SRCS=	hpacel_acpi.c

COPTS.hpacel_acpi.c+=       ${CC_WNO_MAYBE_UNINITIALIZED}

.include <bsd.kmodule.mk>
