#	$NetBSD: Makefile,v 1.2 1994/10/26 02:05:39 cgd Exp $

#	@(#)Makefile	5.5 (Berkeley) 5/11/90

PROG=	mount
SRCS=   mount.c mount_ufs.c getmntopts.c
CFLAGS+= -DLETS_GET_SMALL
NOMAN=

BINDIR=/sbin

.PATH:	${.CURDIR}/../../../../../sbin/mount

.include <bsd.prog.mk>
