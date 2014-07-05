#	$NetBSD: Makefile,v 1.45 2014/07/05 19:22:03 dholland Exp $
#	@(#)Makefile	8.2 (Berkeley) 4/27/95

.include <bsd.own.mk>

PROG=	fsck_ffs
MAN=	fsck_ffs.8
SRCS=	dir.c inode.c main.c pass1.c pass1b.c pass2.c pass3.c pass4.c \
	pass5.c pass6.c fsutil.c setup.c utilities.c ffs_bswap.c ffs_subr.c \
	ffs_tables.c ffs_appleufs.c partutil.c snapshot.c quota2.c quota2_subr.c

FSCK=	${NETBSDSRCDIR}/sbin/fsck
DUMP=	${NETBSDSRCDIR}/sbin/dump
CPPFLAGS+=-I${FSCK} -I${DUMP}
.ifndef  SMALLPROG
CPPFLAGS+=-DPROGRESS
.endif
SRCS+=	progress.c
.PATH:	${FSCK}

.PATH:	${NETBSDSRCDIR}/sys/ufs/ffs ${NETBSDSRCDIR}/sys/ufs/ufs ${FSCK} ${DUMP}

SRCS+= vfs_wapbl.c wapbl.c
.PATH:	${NETBSDSRCDIR}/sys/kern
CPPFLAGS+=-DWAPBL_DEBUG_PRINT=0

LDADD+=-lutil
DPADD+=${LIBUTIL}

LDADD+=-lprop
DPADD+=${LIBPROP}

COPTS.ffs_appleufs.c+=	-Wno-pointer-sign

.if ${MACHINE_ARCH} == "m68000"
COPTS.pass1.c+=	-fno-tree-fre -fno-tree-lrs
.endif

SUBDIR+=SMM.doc

.include <bsd.prog.mk>
.include <bsd.subdir.mk>
