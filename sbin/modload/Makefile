#	$NetBSD: Makefile,v 1.10 2008/02/09 19:33:00 jmmv Exp $

.include <bsd.own.mk>

.if ${MKMODULAR} != "no"	# LKMS

PROG=	modload
SRCS=	modload.c
MAN=	modload.8

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
