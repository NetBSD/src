#	$NetBSD: Makefile,v 1.8.12.1 2008/02/18 21:04:17 mjf Exp $

.include <bsd.own.mk>

.if ${MKMODULAR} != "no"	# new module framework

PROG=	modload
SRCS=	main.c
MAN=	modload.8

.else				# LKMS

PROG=	modload
SRCS=	modload.c
MAN=	modload.8

.if (${OBJECT_FMT} == "ELF")
SRCS+= elf.c
.else
SRCS+= a.out.c
CFLAGS+=-DUSE_AOUT
.endif

.endif

.include <bsd.prog.mk>
