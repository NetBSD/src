#	$NetBSD: Makefile,v 1.5 2002/08/19 10:16:53 lukem Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/5/93

.include <bsd.own.mk>

PROG=	fsck_lfs
MAN=	fsck_lfs.8
SRCS=	dir.c inode.c main.c pass1.c pass2.c pass3.c pass4.c \
	fsutil.c setup.c utilities.c lfs_cksum.c vars.c
SRCS+=  pass0.c pass5.c
FSCK=   ${NETBSDSRCDIR}/sbin/fsck
.PATH:	${NETBSDSRCDIR}/sys/ufs/lfs ${FSCK}
CPPFLAGS+=-I${FSCK} # -DVERBOSE_BLOCKMAP

.include <bsd.prog.mk>
