#	$NetBSD: Makefile,v 1.11 2001/12/02 08:41:14 jmc Exp $

PROG=		ktruss
SRCS=		ktrace.c dump.c subr.c ioctl.c misc.c setemul.c
CPPFLAGS+=	-I. -I${.CURDIR}/../ktrace -I${.CURDIR}/../../sys \
		-I${.CURDIR}/../kdump -DKTRUSS
LDSTATIC?=-static
CLEANFILES+=	misc.c misc.h ioctl.c
MAN=
WFORMAT=1
dump.c misc.c: misc.h
misc.h: ${DESTDIR}/usr/include/sys/errno.h \
	       ${DESTDIR}/usr/include/sys/signal.h
	CPP=${CPP:Q} CPPFLAGS=${CPPFLAGS:Q} sh ${.CURDIR}/makeerrnos.sh \
	    ${DESTDIR}/usr/include/sys/errno.h \
	    ${DESTDIR}/usr/include/sys/signal.h misc

# XXX It looks like .NOTPARALLEL doesn't works.
.NOTPARALLEL:

.include "${.CURDIR}/../kdump/Makefile.ioctl-c"
.include <bsd.prog.mk>

.PATH: ${.CURDIR}/../ktrace ${.CURDIR}/../kdump
