#	from: @(#)Makefile	8.1 (Berkeley) 6/5/93
#	$Id: Makefile,v 1.7 1994/06/25 05:58:04 cgd Exp $

PROG=	fsck
MAN8=	fsck.0
SRCS=	dir.c inode.c main.c pass1.c pass1b.c pass2.c pass3.c pass4.c \
	pass5.c preen.c setup.c utilities.c ffs_subr.c ffs_tables.c
.PATH:	${.CURDIR}/../../sys/ufs/ffs

.if !make(all) && !make(depend) && !make(lint) && !make(tags)
SUBDIR+= SMM.doc
.endif

.include <bsd.prog.mk>
