#	$NetBSD: Makefile,v 1.36 2013/11/09 21:39:27 christos Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/5/93

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

.PATH:	${NETBSDSRCDIR}/sys/ufs/ufs
SRCS+=	quota2_subr.c

.PATH:	${NETBSDSRCDIR}/sys/kern
CPPFLAGS+=-DWAPBL_DEBUG_PRINT=0

SRCS+=	quota2.c

LDADD+= -lutil -ledit -lterminfo -lprop
.ifndef HOSTPROG
DPADD+= ${LIBUTIL} ${LIBEDIT} ${LIBTERMINFO} ${LIBPROP}
.endif

.for f in fsdb ffs_appleufs
COPTS.${f}.c+=	-Wno-pointer-sign
.endfor

.if ${MACHINE_ARCH} == "m68000"
COPTS.pass1.c+=	-fno-tree-fre -fno-tree-lrs
.endif

.include <bsd.prog.mk>

# XXX
.if defined(HAVE_GCC) && ${HAVE_GCC} == 45 && ${MACHINE_ARCH} == "vax"
COPTS.fsdb.c+=	-O0
.endif
