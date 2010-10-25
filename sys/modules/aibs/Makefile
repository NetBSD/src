#	$NetBSD: Makefile,v 1.1 2010/10/25 17:17:57 jruoho Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/acpi

KMOD=	aibs
IOCONF=	AIBS.ioconf
SRCS=	atk0110.c

WARNS=	3

.include <bsd.kmodule.mk>
