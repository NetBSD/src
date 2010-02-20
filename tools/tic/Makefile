#	$NetBSD: Makefile,v 1.2 2010/02/20 17:02:33 roy Exp $

HOSTPROGNAME=	${_TOOL_PREFIX}tic
HOST_SRCDIR=	usr.bin/tic

.include "${.CURDIR}/../Makefile.host"

.PATH:		${NETBSDSRCDIR}/lib/libterminfo
SRCS+=		hash.c

tic:		hash.lo
