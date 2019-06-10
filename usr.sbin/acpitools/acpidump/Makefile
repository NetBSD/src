# $NetBSD: Makefile,v 1.6.16.1 2019/06/10 22:10:27 christos Exp $
# $FreeBSD: src/usr.sbin/acpi/acpidump/Makefile,v 1.7 2003/08/28 03:33:07 njl Exp $

.if (${MACHINE_ARCH} == "i386" || ${MACHINE_ARCH} == "x86_64" || ${MACHINE_ARCH} == "aarch64")
CPPFLAGS+=-I${.CURDIR}/../../../sys -D_KERNTYPES
CPPFLAGS+=-I${.CURDIR}/..
PROG=	acpidump
SRCS=	acpi.c acpi_user.c acpidump.c
.endif

MAN=	acpidump.8
.include <bsd.prog.mk>
