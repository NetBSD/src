# $NetBSD: Makefile,v 1.5 2009/12/27 21:42:22 abs Exp $
# $FreeBSD: src/usr.sbin/acpi/acpidump/Makefile,v 1.7 2003/08/28 03:33:07 njl Exp $

.if (${MACHINE_ARCH} == "i386" || ${MACHINE_ARCH} == "x86_64")
CPPFLAGS+=-I${.CURDIR}/../../../sys
CPPFLAGS+=-I${.CURDIR}/..
PROG=	acpidump
SRCS=	acpi.c acpi_user.c acpidump.c
.endif

MAN=	acpidump.8
.include <bsd.prog.mk>
