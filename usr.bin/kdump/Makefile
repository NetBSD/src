#	$NetBSD: Makefile,v 1.8 1997/07/21 19:29:58 phil Exp $
#	from: @(#)Makefile	8.1 (Berkeley) 6/6/93

LDSTATIC=-static
PROG=	kdump
CFLAGS+=-I${.CURDIR}/../ktrace
SRCS=	kdump.c ioctl.c subr.c
.PATH:	${.CURDIR}/../ktrace
CLEANFILES+=ioctl.c

ioctl.c: mkioctls ${DESTDIR}/usr/include/sys/ioctl.h ${DESTDIR}/usr/include/sys/ioctl_compat.h
	/bin/sh ${.CURDIR}/mkioctls ${DESTDIR}/usr/include/sys/ioctl.h \
	    ${DESTDIR}/usr/include/sys/ioctl_compat.h > ioctl.c

.include <bsd.prog.mk>
