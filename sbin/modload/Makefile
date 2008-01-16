#	$NetBSD: Makefile,v 1.9 2008/01/16 12:34:58 ad Exp $

.if ("1" == "1")	# LKMS

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

.else			# new module framework

PROG=	modload
SRCS=	main.c
MAN=	modload.8

.include <bsd.prog.mk>

.endif
