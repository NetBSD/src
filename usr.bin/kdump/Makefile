#	$NetBSD: Makefile,v 1.9 1997/08/01 04:37:36 mikel Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/6/93

PROG=	kdump
CFLAGS+=-I${.CURDIR}/../ktrace
SRCS=	kdump.c ioctl.c subr.c
.PATH:	${.CURDIR}/../ktrace
LDSTATIC=-static
CLEANFILES+=ioctl.c

ioctl.c: mkioctls ${DESTDIR}/usr/include/sys/ioctl.h ${DESTDIR}/usr/include/sys/ioctl_compat.h
	/bin/sh ${.CURDIR}/mkioctls ${DESTDIR}/usr/include/sys/ioctl.h \
	    ${DESTDIR}/usr/include/sys/ioctl_compat.h > ioctl.c

.include <bsd.prog.mk>
