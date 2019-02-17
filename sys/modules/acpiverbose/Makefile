#	$NetBSD: Makefile,v 1.5 2019/02/17 04:05:42 rin Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/acpi

KMOD=	acpiverbose
IOCONF=	acpiverbose.ioconf
SRCS=	acpi_verbose.c

WARNS=	3

.include <bsd.kmodule.mk>
