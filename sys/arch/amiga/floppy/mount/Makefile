#	from: @(#)Makefile	5.5 (Berkeley) 5/11/90
#	$Id: Makefile,v 1.1 1994/07/30 19:30:45 chopps Exp $

PROG=	mount
SRCS=   mount.c mount_ufs.c getmntopts.c
CFLAGS+= -DLETS_GET_SMALL
NOMAN=

BINDIR=/sbin

.PATH:	${.CURDIR}/../../../../../sbin/mount

.include <bsd.prog.mk>
