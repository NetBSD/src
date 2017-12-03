#	$NetBSD: Makefile,v 1.7.18.1 2017/12/03 11:35:49 jdolecek Exp $

.include <bsd.init.mk>

PROG=installboot
SRCS=installboot.c chksum.c
BINDIR=/usr/mdec

NOMAN=	# defined

.PATH:	${.CURDIR}/../elf2bb	# chksum.c

.include <bsd.prog.mk>
