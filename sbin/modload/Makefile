#	$NetBSD: Makefile,v 1.7 1999/06/13 12:54:40 mrg Exp $

WARNS=	1
PROG=	modload
SRCS=	modload.c
MAN=	modload.8

.include <bsd.own.mk> # for OBJECT_FMT

.if (${OBJECT_FMT} == "ELF")
SRCS+= elf.c
.else
SRCS+= a.out.c
CFLAGS+=-DUSE_AOUT
.endif

.include <bsd.prog.mk>
