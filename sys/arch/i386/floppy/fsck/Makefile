#	from: @(#)Makefile	5.15 (Berkeley) 6/29/90
#	$Id: Makefile,v 1.2 1994/06/14 01:16:30 cgd Exp $

SRCDIR=	${.CURDIR}/../../../../../sbin/fsck

PROG=	fsck
NOMAN=
SRCS=	dir.c inode.c main.c pass1.c pass1b.c pass2.c pass3.c pass4.c \
	pass5.c preen.c setup.c utilities.c ffs_subr.c ffs_tables.c
CFLAGS+= -DSMALL

BINDIR=	/sbin

.PATH:	${SRCDIR} ${SRCDIR}/../../sys/ufs/ffs

.include <bsd.prog.mk>
