#	$NetBSD: Makefile,v 1.17 2000/11/14 00:10:16 jdolecek Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/6/93

.include <bsd.own.mk>

PROG=	kdump
CPPFLAGS+=-I${.CURDIR}/../ktrace -I${.CURDIR}/../../sys
LDSTATIC?=-static
SRCS=	kdump.c ioctl.c subr.c setemul.c
.PATH:	${.CURDIR}/../ktrace
CLEANFILES+=ioctl.c
WFORMAT=1
.include "Makefile.ioctl-c"

.include <bsd.prog.mk>
