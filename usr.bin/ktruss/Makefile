#	$NetBSD: Makefile,v 1.8 2000/12/30 14:54:40 sommerfeld Exp $

PROG=		ktruss
SRCS=		ktrace.c dump.c subr.c ioctl.c misc.c setemul.c
CPPFLAGS+=	-I. -I${.CURDIR}/../ktrace -I${.CURDIR}/../../sys \
		-I${.CURDIR}/../kdump -DKTRUSS
LDSTATIC?=-static
CLEANFILES+=	misc.c misc.h ioctl.c
MAN=
WFORMAT=1
dump.c: misc.h
misc.c misc.h: ${DESTDIR}/usr/include/sys/errno.h \
	       ${DESTDIR}/usr/include/sys/signal.h
	${.CURDIR}/makeerrnos.sh \
	    ${DESTDIR}/usr/include/sys/errno.h \
	    ${DESTDIR}/usr/include/sys/signal.h misc

.NOTPARALLEL:

.include "${.CURDIR}/../kdump/Makefile.ioctl-c"
.include <bsd.prog.mk>

.PATH: ${.CURDIR}/../ktrace ${.CURDIR}/../kdump
