# $NetBSD: Makefile,v 1.2 2007/01/14 04:44:11 christos Exp $
# Id: Makefile,v 1.5 2000/07/14 18:16:30 iwasaki Exp 
# $FreeBSD: src/usr.sbin/acpi/amldb/Makefile,v 1.7 2001/10/22 17:25:32 iwasaki Exp $

.if (${MACHINE} == "i386" || ${MACHINE} == "amd64")
PROG=	amldb
SRCS=	amldb.c debug.c region.c
SRCS+=	aml_parse.c aml_name.c aml_amlmem.c aml_memman.c aml_store.c \
	aml_obj.c aml_evalobj.c aml_common.c

CPPFLAGS+= -I${.CURDIR}
.endif

MAN=	amldb.8

.include <bsd.prog.mk>
.PATH: ${.CURDIR}/../aml
