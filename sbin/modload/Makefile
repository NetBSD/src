#	$NetBSD: Makefile,v 1.8 2005/06/27 01:00:05 christos Exp $

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
