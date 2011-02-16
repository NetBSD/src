#	$NetBSD: Makefile,v 1.3 2011/02/16 08:08:14 jruoho Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/acpi

KMOD=	aibs
IOCONF=	aibs.ioconf
SRCS=	atk0110.c

WARNS=	4

.include <bsd.kmodule.mk>
