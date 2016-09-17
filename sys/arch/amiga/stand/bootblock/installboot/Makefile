#	$NetBSD: Makefile,v 1.8 2016/09/17 17:09:10 christos Exp $

.include <bsd.init.mk>

PROG=installboot
SRCS=installboot.c chksum.c
BINDIR=/usr/mdec

NOMAN=	# defined

.PATH:	${.CURDIR}/../elf2bb	# chksum.c

.include <bsd.prog.mk>
