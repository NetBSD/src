#	$NetBSD: Makefile,v 1.16 2000/11/13 21:48:33 jdolecek Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/6/93

.include <bsd.own.mk>

PROG=	kdump
CPPFLAGS+=-I${.CURDIR}/../ktrace -I${.CURDIR}/../../sys
SRCS=	kdump.c ioctl.c subr.c setemul.c
.PATH:	${.CURDIR}/../ktrace
CLEANFILES+=ioctl.c
WFORMAT=1
.include "Makefile.ioctl-c"

.include <bsd.prog.mk>
