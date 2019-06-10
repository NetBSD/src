#	$NetBSD: Makefile,v 1.4.56.1 2019/06/10 22:09:08 christos Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/acpi

KMOD=	acpiverbose
IOCONF=	acpiverbose.ioconf
SRCS=	acpi_verbose.c

WARNS=	3

.include <bsd.kmodule.mk>
