# $NetBSD: Makefile,v 1.3 2011/02/16 08:35:51 jruoho Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/acpi

KMOD=	acpiacad
IOCONF=	acpiacad.ioconf
SRCS=	acpi_acad.c

WARNS=	4

.include <bsd.kmodule.mk>
