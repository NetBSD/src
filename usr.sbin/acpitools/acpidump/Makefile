# $NetBSD: Makefile,v 1.4 2009/12/22 08:44:03 cegger Exp $
# $FreeBSD: src/usr.sbin/acpi/acpidump/Makefile,v 1.7 2003/08/28 03:33:07 njl Exp $

.if (${MACHINE_ARCH} == "i386" || ${MACHINE_ARCH} == "x86_64")
CPPFLAGS+=-I${.CURDIR}/../../../sys
CPPFLAGS+=-I${.CURDIR}/..
PROG=	acpidump
MAN=	acpidump.8
SRCS=	acpi.c acpi_user.c acpidump.c
.endif

.include <bsd.prog.mk>
