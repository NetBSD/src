#	$NetBSD: Makefile,v 1.14.4.1 2000/10/18 01:32:44 tv Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/6/93

.include <bsd.own.mk>

PROG=	kdump
CPPFLAGS+=-I${.CURDIR}/../ktrace -I${.CURDIR}/../../sys
SRCS=	kdump.c ioctl.c subr.c setemul.c
.PATH:	${.CURDIR}/../ktrace
LDSTATIC?=-static
CLEANFILES+=ioctl.c
WFORMAT=1
.include "Makefile.ioctl-c"

.include <bsd.prog.mk>
