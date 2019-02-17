# $NetBSD: Makefile,v 1.2 2019/02/17 04:05:50 rin Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/acpi

KMOD=	hpacel
IOCONF=	hpacel.ioconf
SRCS=	hpacel_acpi.c

.include <bsd.kmodule.mk>
