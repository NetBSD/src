#	$NetBSD: Makefile,v 1.21 2002/09/19 11:31:21 lukem Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/6/93

.include <bsd.own.mk>		# for MKDYNAMICROOT & NETBSDSRCDIR

PROG=	kdump
CPPFLAGS+=-I${NETBSDSRCDIR}/usr.bin/ktrace -I${NETBSDSRCDIR}/sys
SRCS=	kdump.c ioctl.c subr.c setemul.c
.PATH:	${NETBSDSRCDIR}/usr.bin/ktrace
CLEANFILES+=ioctl.c
WFORMAT=1

.if (${MKDYNAMICROOT} == "no")
LDSTATIC?=-static
.endif

.include "Makefile.ioctl-c"

.include <bsd.prog.mk>
