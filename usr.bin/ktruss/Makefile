#	$NetBSD: Makefile,v 1.10 2001/01/16 02:46:25 cgd Exp $

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
	CPP=${CPP:Q} CPPFLAGS=${CPPFLAGS:Q} ${.CURDIR}/makeerrnos.sh \
	    ${DESTDIR}/usr/include/sys/errno.h \
	    ${DESTDIR}/usr/include/sys/signal.h misc

# XXX It looks like .NOTPARALLEL doesn't works.
.NOTPARALLEL:

.include "${.CURDIR}/../kdump/Makefile.ioctl-c"
.include <bsd.prog.mk>

.PATH: ${.CURDIR}/../ktrace ${.CURDIR}/../kdump
