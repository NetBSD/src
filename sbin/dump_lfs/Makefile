#	$NetBSD: Makefile,v 1.4 2002/08/19 10:16:53 lukem Exp $
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
CPPFLAGS+=-I${NETBSDSRCDIR}/usr.bin/who -DSUPPORT_UTMPX -DSUPPORT_UTMP
# CPPFLAGS+= -DDEBUG -DTDEBUG -DFDEBUG -DWRITEDEBUG -DSTATS -DDIAGNOSTICS
SRCS=	itime.c main.c optr.c dumprmt.c rcache.c tape.c traverse.c unctime.c \
	ffs_bswap.c lfs_inode.c utmpentry.c
BINGRP=	tty
BINMODE=2555
MAN=	dump_lfs.8
MLINKS+=dump_lfs.8 rdump_lfs.8
CFLAGS+=-g

.PATH:  ${NETBSDSRCDIR}/sbin/dump ${NETBSDSRCDIR}/sys/ufs/lfs \
	${NETBSDSRCDIR}/sys/ufs/ffs ${NETBSDSRCDIR}/usr.bin/who

.include <bsd.prog.mk>
