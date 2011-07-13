# $NetBSD: Makefile,v 1.1 2011/07/13 07:52:49 jruoho Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/acpi

KMOD=	hpacel
IOCONF=	hpacel.ioconf
SRCS=	hpacel_acpi.c

WARNS=	4

.include <bsd.kmodule.mk>
