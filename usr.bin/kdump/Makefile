#	$NetBSD: Makefile,v 1.24 2003/11/16 10:10:37 manu Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/6/93

.include <bsd.own.mk>		# for MKDYNAMICROOT & NETBSDSRCDIR

PROG=	kdump
CPPFLAGS+=-I${NETBSDSRCDIR}/usr.bin/ktrace -I${NETBSDSRCDIR}/sys
SRCS=	kdump.c ioctl.c subr.c setemul.c siginfo.c
.PATH:	${NETBSDSRCDIR}/usr.bin/ktrace
CLEANFILES+=ioctl.c siginfo.c
WFORMAT=1

.if (${MKDYNAMICROOT} == "no")
LDSTATIC?=-static
.endif

.include "Makefile.ioctl-c"
.include "Makefile.siginfo-c"

.include <bsd.prog.mk>
