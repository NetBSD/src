#	$NetBSD: Makefile,v 1.3 2000/05/30 04:33:14 perseant Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/5/93

PROG=	fsck_lfs
MAN=	fsck_lfs.8
SRCS=	dir.c inode.c main.c pass1.c pass2.c pass3.c pass4.c \
	fsutil.c setup.c utilities.c lfs_cksum.c vars.c
SRCS+=  pass0.c pass5.c
FSCK=   ${.CURDIR}/../fsck
.PATH:	${.CURDIR}/../../sys/ufs/lfs ${FSCK}
CPPFLAGS+=-I${FSCK} -DVERBOSE_BLOCKMAP

.include <bsd.prog.mk>
