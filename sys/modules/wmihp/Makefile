# $NetBSD: Makefile,v 1.2.4.1 2011/06/06 09:09:51 jruoho Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/acpi/wmi

KMOD=   wmihp
IOCONF=	wmihp.ioconf
SRCS=	wmi_hp.c

WARNS=	4

.include <bsd.kmodule.mk>
