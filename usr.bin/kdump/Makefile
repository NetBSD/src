#	$NetBSD: Makefile,v 1.11 1998/03/02 19:42:25 cgd Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/6/93

.include <bsd.own.mk>

PROG=	kdump
CPPFLAGS+=-I${.CURDIR}/../ktrace
SRCS=	kdump.c ioctl.c subr.c
.PATH:	${.CURDIR}/../ktrace
LDSTATIC?=-static
CLEANFILES+=ioctl.c

ioctl.c: mkioctls ${DESTDIR}/usr/include/sys/ioctl.h ${DESTDIR}/usr/include/sys/ioctl_compat.h
	/bin/sh ${.CURDIR}/mkioctls ${DESTDIR}/usr/include/sys/ioctl.h \
	    ${DESTDIR}/usr/include/sys/ioctl_compat.h > ioctl.c

.include <bsd.prog.mk>
