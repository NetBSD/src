#	$NetBSD: Makefile,v 1.6 1997/05/06 20:45:33 gwr Exp $
#	from: @(#)Makefile	8.1 (Berkeley) 6/6/93

LDSTATIC=-static
PROG=	kdump
CFLAGS+=-I${.CURDIR}/../ktrace
SRCS=	kdump.c ioctl.c subr.c
.PATH.c: ${.CURDIR}/../ktrace
CLEANFILES+=ioctl.c

ioctl.c: mkioctls
	/bin/sh ${.CURDIR}/mkioctls ${DESTDIR}/usr/include/sys/ioctl.h \
	    ${DESTDIR}/usr/include/sys/ioctl_compat.h > ioctl.c

.include <bsd.prog.mk>
