#	$NetBSD: Makefile,v 1.40 2009/06/05 21:52:31 haad Exp $
#	@(#)Makefile	8.2 (Berkeley) 4/27/95

WARNS?=	3	# XXX: sign-compare issues

.include <bsd.own.mk>

PROG=	fsck_ffs
MAN=	fsck_ffs.8
SRCS=	dir.c inode.c main.c pass1.c pass1b.c pass2.c pass3.c pass4.c \
	pass5.c fsutil.c setup.c utilities.c ffs_bswap.c ffs_subr.c \
	ffs_tables.c ffs_appleufs.c partutil.c snapshot.c

FSCK=	${NETBSDSRCDIR}/sbin/fsck
DUMP=	${NETBSDSRCDIR}/sbin/dump
CPPFLAGS+=-I${FSCK} -I${DUMP}
.ifndef  SMALLPROG
CPPFLAGS+=-DPROGRESS
.endif
SRCS+=	progress.c
.PATH:	${FSCK}

.PATH:	${NETBSDSRCDIR}/sys/ufs/ffs ${FSCK} ${DUMP}

SRCS+= vfs_wapbl.c wapbl.c
.PATH:	${NETBSDSRCDIR}/sys/kern
CPPFLAGS+=-DWAPBL_DEBUG_PRINT=0

LDADD+=-lutil
DPADD+=${LIBUTIL}

LDADD+=-lprop
DPADD+=${LIBPROP}

.if (defined(HAVE_GCC) && ${HAVE_GCC} == 4) || defined(HAVE_PCC)
COPTS.ffs_appleufs.c+=	-Wno-pointer-sign
.endif

.if ${MACHINE_ARCH} == "m68000"
COPTS.pass1.c+=	-fno-tree-fre -fno-tree-lrs
.endif

.if make(install)
SUBDIR+=SMM.doc
.endif

.include <bsd.prog.mk>
.include <bsd.subdir.mk>
