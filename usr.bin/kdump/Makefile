#	$NetBSD: Makefile,v 1.14 2000/04/10 09:13:45 jdolecek Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/6/93

.include <bsd.own.mk>

PROG=	kdump
CPPFLAGS+=-I${.CURDIR}/../ktrace -I${.CURDIR}/../../sys
SRCS=	kdump.c ioctl.c subr.c setemul.c
.PATH:	${.CURDIR}/../ktrace
LDSTATIC?=-static
CLEANFILES+=ioctl.c

.include "Makefile.ioctl-c"

.include <bsd.prog.mk>
