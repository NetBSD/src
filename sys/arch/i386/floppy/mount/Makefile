#	from: @(#)Makefile	5.5 (Berkeley) 5/11/90
#	$Id: Makefile,v 1.1 1994/04/18 08:47:28 cgd Exp $

PROG=	mount
SRCS=	mount.c
CFLAGS+= -DLETS_GET_SMALL
NOMAN=

BINDIR=/sbin

.PATH:	${.CURDIR}/../../../../../sbin/mount

.include <bsd.prog.mk>
