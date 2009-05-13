#	$NetBSD: Makefile,v 1.24.4.1 2009/05/13 19:19:01 jym Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/5/93

WARNS?=	3	# XXX: sign-compare issues in ../fsck_ffs

.include <bsd.own.mk>

PROG=	fsdb
MAN=	fsdb.8
SRCS=	fsdb.c fsdbutil.c
SRCS+=	dir.c inode.c pass1.c pass1b.c pass2.c pass3.c pass4.c \
	pass5.c setup.c utilities.c
SRCS+=	ffs_bswap.c ffs_subr.c ffs_tables.c ffs_appleufs.c
SRCS+=	fsutil.c partutil.c

FSCK=	${NETBSDSRCDIR}/sbin/fsck
FSCK_FFS=${NETBSDSRCDIR}/sbin/fsck_ffs
CPPFLAGS+= -I${FSCK} -I${FSCK_FFS}
.PATH:	${FSCK} ${FSCK_FFS} ${NETBSDSRCDIR}/sys/ufs/ffs

SRCS+=	vfs_wapbl.c wapbl.c
.PATH:	${NETBSDSRCDIR}/sys/kern
CPPFLAGS+=-DWAPBL_DEBUG_PRINT=0

LDADD+= -lutil -ledit -ltermcap
.ifndef HOSTPROG
DPADD+= ${LIBUTIL} ${LIBEDIT} ${LIBTERMCAP}
.endif

.if (defined(HAVE_GCC) && ${HAVE_GCC} == 4) || defined(HAVE_PCC)
.for f in fsdb ffs_appleufs
COPTS.${f}.c+=	-Wno-pointer-sign
.endfor
.endif

.if ${MACHINE_ARCH} == "m68000"
COPTS.pass1.c+=	-fno-tree-fre -fno-tree-lrs
.endif

.include <bsd.prog.mk>
