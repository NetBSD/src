#	$NetBSD: Makefile,v 1.12.2.1 2008/03/24 07:14:49 keiichi Exp $

.include <bsd.own.mk>

.if ${MKMODULAR} != "no"	# new module framework

PROG=	modload
SRCS=	main.c
MAN=	modload.8
LDADD=	-lprop

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
