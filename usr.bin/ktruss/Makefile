#	$NetBSD: Makefile,v 1.6 2000/11/13 21:48:34 jdolecek Exp $

PROG=		ktruss
SRCS=		ktrace.c dump.c subr.c ioctl.c misc.c setemul.c
CPPFLAGS+=	-I. -I${.CURDIR}/../ktrace -I${.CURDIR}/../../sys \
		-I${.CURDIR}/../kdump -DKTRUSS
CLEANFILES+=	misc.c misc.h ioctl.c
MAN=
WFORMAT=1
dump.c: misc.h
misc.c misc.h: ${DESTDIR}/usr/include/sys/errno.h \
	       ${DESTDIR}/usr/include/sys/signal.h
	${.CURDIR}/makeerrnos.sh \
	    ${DESTDIR}/usr/include/sys/errno.h \
	    ${DESTDIR}/usr/include/sys/signal.h misc

.include "${.CURDIR}/../kdump/Makefile.ioctl-c"
.include <bsd.prog.mk>

.PATH: ${.CURDIR}/../ktrace ${.CURDIR}/../kdump
