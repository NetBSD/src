# $NetBSD: Makefile,v 1.1.2.1 2011/06/06 09:09:45 jruoho Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/acpi

KMOD=	acpipmtr
IOCONF=	acpipmtr.ioconf
SRCS=	acpi_pmtr.c

WARNS=	4

.include <bsd.kmodule.mk>
