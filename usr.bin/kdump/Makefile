#	$NetBSD: Makefile,v 1.12.2.1 2000/10/19 16:27:50 he Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/6/93

.include <bsd.own.mk>

PROG=	kdump
CPPFLAGS+=-I${.CURDIR}/../ktrace -I${.CURDIR}/../../sys
SRCS=	kdump.c ioctl.c subr.c
.PATH:	${.CURDIR}/../ktrace
LDSTATIC?=-static
CLEANFILES+=ioctl.c
WFORMAT=1
.include "Makefile.ioctl-c"

ioctl.c: mkioctls ${DESTDIR}/usr/include/sys/ioctl.h ${DESTDIR}/usr/include/sys/ioctl_compat.h
	/bin/sh ${.CURDIR}/mkioctls ${DESTDIR}/usr/include/sys/ioctl.h \
	    ${DESTDIR}/usr/include/sys/ioctl_compat.h > ioctl.c

.include <bsd.prog.mk>
