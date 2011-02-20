# $NetBSD: Makefile,v 1.1 2011/02/20 08:37:50 jruoho Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/acpi

KMOD=	fujbp
IOCONF=	fujbp.ioconf
SRCS=	fujbp_acpi.c

WARNS=	4

.include <bsd.kmodule.mk>
