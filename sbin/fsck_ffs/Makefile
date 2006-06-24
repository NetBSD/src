#	$NetBSD: Makefile,v 1.30 2006/06/24 05:21:11 mrg Exp $
#	@(#)Makefile	8.2 (Berkeley) 4/27/95

.include <bsd.own.mk>

PROG=	fsck_ffs
MAN=	fsck_ffs.8
SRCS=	dir.c inode.c main.c pass1.c pass1b.c pass2.c pass3.c pass4.c \
	pass5.c fsutil.c setup.c utilities.c ffs_bswap.c ffs_subr.c \
	ffs_tables.c ffs_appleufs.c

FSCK=	${NETBSDSRCDIR}/sbin/fsck
CPPFLAGS+=-I${FSCK}
.ifndef  SMALLPROG
CPPFLAGS+=-DPROGRESS
.endif
SRCS+=	progress.c
.PATH:	${FSCK}

.PATH:	${NETBSDSRCDIR}/sys/ufs/ffs ${FSCK}

.if ${HAVE_GCC} == 4
COPTS.ffs_appleufs.c+=	-Wno-pointer-sign
.endif

.if ${HAVE_GCC} == 4 && ${MACHINE_ARCH} == "m68000"
COPTS.pass1.c+=	-O0
.endif

.if make(install)
SUBDIR+=SMM.doc
.endif

.include <bsd.prog.mk>
.include <bsd.subdir.mk>
