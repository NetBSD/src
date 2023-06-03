#	$NetBSD: Makefile,v 1.21 2023/06/03 09:09:12 lukem Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/5/93

#	lfs_inode.c		LFS filestore-specific routines
#
#	DEBUG			use local directory to find ddate and dumpdates
#	TDEBUG			trace out the process forking

WARNS?=	3	# XXX: sign-compare issues

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
#CFLAGS+=-g
DPADD+= ${LIBUTIL}
LDADD+= -lutil

.if ${MACHINE_ARCH} == "m68000"
COPTS.lfs_inode.c+=	-fno-tree-ter
.endif

.if ${MACHINE_ARCH} == "ia64"
COPTS.lfs_inode.c+=	-O3
.endif

.PATH:  ${NETBSDSRCDIR}/sbin/dump ${NETBSDSRCDIR}/sys/ufs/lfs \
    ${NETBSDSRCDIR}/sys/ufs/ffs

CWARNFLAGS.gcc+=	${CC_WNO_FORMAT_TRUNCATION}

.include <bsd.prog.mk>
