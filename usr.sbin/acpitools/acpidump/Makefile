# Id: Makefile,v 1.2 2000/07/14 18:16:29 iwasaki Exp 
# $FreeBSD: src/usr.sbin/acpi/acpidump/Makefile,v 1.6 2001/10/22 17:25:25 iwasaki Exp $

.if (${MACHINE_ARCH} == "i386" || ${MACHINE_ARCH} == "x86_64")
PROG=	acpidump
SRCS=	acpi.c acpi_user.c asl_dump.c aml_dump.c acpidump.c
SRCS+=	aml_parse.c aml_name.c aml_amlmem.c aml_memman.c aml_store.c \
	aml_obj.c aml_evalobj.c aml_common.c

CPPFLAGS+=-I${.CURDIR}/../amldb
.endif

MAN=	acpidump.8
.include <bsd.prog.mk>
.PATH:	${.CURDIR}/../aml
