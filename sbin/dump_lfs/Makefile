#	$NetBSD: Makefile,v 1.8.4.1 2008/06/02 13:21:21 mjf Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/5/93

#	lfs_inode.c		LFS filestore-specific routines
#
#	DEBUG			use local directory to find ddate and dumpdates
#	TDEBUG			trace out the process forking

.include <bsd.own.mk>

PROG=	dump_lfs
LINKS=	${BINDIR}/dump_lfs ${BINDIR}/rdump_lfs
CPPFLAGS+=-DRDUMP -I${.CURDIR} -I${NETBSDSRCDIR}/sbin/dump
CPPFLAGS+=-DDUMP_LFS
# CPPFLAGS+= -DDEBUG -DTDEBUG -DFDEBUG -DWRITEDEBUG -DSTATS -DDIAGNOSTICS
SRCS=	itime.c main.c optr.c dumprmt.c rcache.c snapshot.c tape.c \
	traverse.c unctime.c ffs_bswap.c lfs_inode.c
MAN=	dump_lfs.8
MLINKS+=dump_lfs.8 rdump_lfs.8
CFLAGS+=-g

.if ${MACHINE_ARCH} == "m68000"
COPTS.lfs_inode.c+=	-fno-tree-ter
.endif

.PATH:  ${NETBSDSRCDIR}/sbin/dump ${NETBSDSRCDIR}/sys/ufs/lfs \
	${NETBSDSRCDIR}/sys/ufs/ffs

.include <bsd.prog.mk>
