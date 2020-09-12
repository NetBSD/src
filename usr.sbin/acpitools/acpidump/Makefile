# $NetBSD: Makefile,v 1.9 2020/09/12 15:25:42 jmcneill Exp $
# $FreeBSD: src/usr.sbin/acpi/acpidump/Makefile,v 1.7 2003/08/28 03:33:07 njl Exp $

.include <bsd.init.mk>

.if ${HAVE_ACPI:Uno} == "yes"
CPPFLAGS+=-I${.CURDIR}/../../../sys -D_KERNTYPES
CPPFLAGS+=-I${.CURDIR}/..
PROG=	acpidump
SRCS=	acpi.c acpi_user.c acpidump.c
.endif

MAN=	acpidump.8
.include <bsd.prog.mk>
