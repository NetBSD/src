#	$NetBSD: Makefile,v 1.31 2015/09/26 03:31:11 christos Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/6/93

.include <bsd.own.mk>		# for MKDYNAMICROOT & NETBSDSRCDIR

USE_FORT?= yes	# cryptographic software

PROG=	kdump
SRCS=	kdump.c subr.c setemul.c siginfo.c
.PATH:	${NETBSDSRCDIR}/usr.bin/ktrace
CLEANFILES+=siginfo.c

.if (${MKDYNAMICROOT} == "no")
LDSTATIC?=-static
.endif

.include "Makefile.ioctl-c"
.include "Makefile.siginfo-c"

CPPFLAGS+=-I${NETBSDSRCDIR}/usr.bin/ktrace -I${NETBSDSRCDIR}/sys

LDADD+=-lutil
DPADD+=${LIBUTIL}

.include <bsd.prog.mk>
