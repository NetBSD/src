#	from: @(#)Makefile	5.2 (Berkeley) 4/27/91
#	$Id: Makefile,v 1.6 1994/01/07 12:35:30 deraadt Exp $

PROG=	kdump
CFLAGS+=-I${.CURDIR}/../../../sys -I${.CURDIR}/../ktrace
SRCS=	kdump.c ioctl.c subr.c
.PATH:	${.CURDIR}/../ktrace
CLEANFILES+=ioctl.c

ioctl.c: mkioctls
	/bin/sh ${.CURDIR}/mkioctls > ioctl.c

.include "../../Makefile.inc"
.include <bsd.prog.mk>
