#	$NetBSD: Makefile,v 1.1.4.1 1999/12/27 18:30:22 wrstuden Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/5/93

#	lfs_inode.c		LFS filestore-specific routines
#
#	DEBUG			use local directory to find ddate and dumpdates
#	TDEBUG			trace out the process forking

PROG=	dump_lfs
LINKS=	${BINDIR}/dump_lfs ${BINDIR}/rdump_lfs
CPPFLAGS+=-DRDUMP -I${.CURDIR} -I${.CURDIR}/../dump
# CPPFLAGS+= -DDEBUG -DTDEBUG -DFDEBUG -DWRITEDEBUG -DSTATS -DDIAGNOSTICS
SRCS=	itime.c main.c optr.c dumprmt.c rcache.c tape.c traverse.c unctime.c \
	ffs_bswap.c lfs_inode.c
BINGRP=	tty
BINMODE=2555
MAN=	dump_lfs.8
MLINKS+=dump_lfs.8 rdump_lfs.8
CFLAGS+=-g

.PATH:  ${.CURDIR}/../dump ${.CURDIR}/../../sys/ufs/lfs ${.CURDIR}/../../sys/ufs/ffs

.include <bsd.prog.mk>
