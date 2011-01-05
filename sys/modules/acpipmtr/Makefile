# $NetBSD: Makefile,v 1.1 2011/01/05 20:15:52 jruoho Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/acpi

KMOD=	acpipmtr
IOCONF=	ACPIPMTR.ioconf
SRCS=	acpi_pmtr.c

WARNS=	4

.include <bsd.kmodule.mk>
