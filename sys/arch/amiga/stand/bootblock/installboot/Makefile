#	$NetBSD: Makefile,v 1.7.36.1 2016/10/05 20:55:24 skrll Exp $

.include <bsd.init.mk>

PROG=installboot
SRCS=installboot.c chksum.c
BINDIR=/usr/mdec

NOMAN=	# defined

.PATH:	${.CURDIR}/../elf2bb	# chksum.c

.include <bsd.prog.mk>
