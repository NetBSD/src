# $NetBSD: Makefile,v 1.1.2.2 2011/03/05 15:10:44 bouyer Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/acpi

KMOD=	fujbp
IOCONF=	fujbp.ioconf
SRCS=	fujbp_acpi.c

WARNS=	4

.include <bsd.kmodule.mk>
