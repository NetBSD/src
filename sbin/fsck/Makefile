#	from: @(#)Makefile	5.15 (Berkeley) 6/29/90
#	$Id: Makefile,v 1.5 1993/08/01 05:37:50 mycroft Exp $

PROG=	fsck
MAN8=	fsck.0
SRCS=	dir.c inode.c main.c pass1.c pass1b.c pass2.c pass3.c pass4.c \
	pass5.c preen.c setup.c utilities.c ufs_subr.c ufs_tables.c
.PATH:	${.CURDIR}/../../sys/ufs

.include <bsd.prog.mk>
