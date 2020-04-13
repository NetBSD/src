# $NetBSD: Makefile,v 1.4.16.2 2020/04/13 08:05:50 martin Exp $
# Id: Makefile,v 1.5 2000/07/14 18:16:30 iwasaki Exp 
# $FreeBSD: src/usr.sbin/acpi/amldb/Makefile,v 1.7 2001/10/22 17:25:32 iwasaki Exp $

.if (${MACHINE_ARCH} == "i386" || ${MACHINE_ARCH} == "x86_64" || ${MACHINE_ARCH} == "aarch64")
PROG=	amldb
SRCS=	amldb.c debug.c region.c
SRCS+=	aml_parse.c aml_name.c aml_amlmem.c aml_memman.c aml_store.c \
	aml_obj.c aml_evalobj.c aml_common.c

CPPFLAGS+= -I${.CURDIR} -D_KERNTYPES
.endif

MAN=	amldb.8

COPTS.aml_name.c+=	${GCC_NO_STRINGOP_TRUNCATION}

.include <bsd.prog.mk>
.PATH: ${.CURDIR}/../aml
