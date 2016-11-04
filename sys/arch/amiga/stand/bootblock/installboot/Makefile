#	$NetBSD: Makefile,v 1.7.40.1 2016/11/04 14:48:58 pgoyette Exp $

.include <bsd.init.mk>

PROG=installboot
SRCS=installboot.c chksum.c
BINDIR=/usr/mdec

NOMAN=	# defined

.PATH:	${.CURDIR}/../elf2bb	# chksum.c

.include <bsd.prog.mk>
