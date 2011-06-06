# $NetBSD: Makefile,v 1.1.8.2 2011/06/06 09:09:51 jruoho Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/acpi/wmi

KMOD=	wmieeepc
IOCONF=	wmieeepc.ioconf
SRCS=	wmi_eeepc.c

WARNS=	4

.include <bsd.kmodule.mk>
