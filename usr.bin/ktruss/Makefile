#	$NetBSD: Makefile,v 1.12 2002/08/27 14:46:18 lukem Exp $

NOMAN=		# defined

.include <bsd.own.mk>		# for MKDYNAMICROOT definition

PROG=		ktruss
SRCS=		ktrace.c dump.c subr.c ioctl.c misc.c setemul.c
CPPFLAGS+=	-I. -I${.CURDIR}/../ktrace -I${.CURDIR}/../../sys \
		-I${.CURDIR}/../kdump -DKTRUSS
CLEANFILES+=	misc.c misc.h ioctl.c
WFORMAT=1

.if (${MKDYNAMICROOT} == "no")
LDSTATIC?=-static
.endif

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
