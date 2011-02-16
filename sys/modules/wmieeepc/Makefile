# $NetBSD: Makefile,v 1.1 2011/02/16 07:30:28 jruoho Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/acpi/wmi

KMOD=	wmieeepc
IOCONF=	wmieeepc.ioconf
SRCS=	wmi_eeepc.c

WARNS=	4

.include <bsd.kmodule.mk>
