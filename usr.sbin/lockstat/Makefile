#	$NetBSD: Makefile,v 1.1 2006/09/07 00:50:45 ad Exp $

.include <bsd.own.mk>

SRCS=	elf32.c elf64.c main.c
PROG=	lockstat
MAN=	lockstat.8

DPADD=	${LIBUTIL} 
LDADD=	-lutil

elf64.o:	elf32.c

.include <bsd.prog.mk>
