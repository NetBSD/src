#	from: @(#)Makefile	5.5 (Berkeley) 5/11/90
#	$Id: Makefile,v 1.2 1994/06/14 01:16:40 cgd Exp $

PROG=	mount
SRCS=   mount.c mount_ufs.c getmntopts.c
CFLAGS+= -DLETS_GET_SMALL
NOMAN=

BINDIR=/sbin

.PATH:	${.CURDIR}/../../../../../sbin/mount

.include <bsd.prog.mk>
