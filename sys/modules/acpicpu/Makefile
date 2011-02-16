# $NetBSD: Makefile,v 1.6 2011/02/16 08:35:52 jruoho Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/acpi
.PATH:	${S}/arch/x86/acpi

KMOD=	acpicpu
IOCONF=	acpicpu.ioconf
SRCS=	acpi_cpu.c \
	acpi_cpu_md.c \
	acpi_cpu_cstate.c \
	acpi_cpu_pstate.c \
	acpi_cpu_tstate.c

WARNS=	 4

.include <bsd.kmodule.mk>
