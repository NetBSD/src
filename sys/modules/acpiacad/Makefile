# $NetBSD: Makefile,v 1.2.2.1 2011/06/06 09:09:44 jruoho Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/acpi

KMOD=	acpiacad
IOCONF=	acpiacad.ioconf
SRCS=	acpi_acad.c

WARNS=	4

.include <bsd.kmodule.mk>
