#	$NetBSD: Makefile,v 1.15 2000/10/11 14:46:05 is Exp $
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
