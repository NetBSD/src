#	$NetBSD: Makefile,v 1.15 2003/02/11 10:43:29 drochner Exp $

NOMAN=		# defined

.include <bsd.own.mk>		# for MKDYNAMICROOT & NETBSDSRCDIR

PROG=		ktruss
SRCS=		ktrace.c dump.c subr.c ioctl.c misc.c setemul.c
CPPFLAGS+=	-I. -I${NETBSDSRCDIR}/usr.bin/ktrace -I${NETBSDSRCDIR}/sys \
		-I${NETBSDSRCDIR}/usr.bin/kdump -DKTRUSS
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

# XXX It looks like .NOTPARALLEL doesn't work.
.NOTPARALLEL:

.include "${NETBSDSRCDIR}/usr.bin/kdump/Makefile.ioctl-c"
.include <bsd.prog.mk>

.PATH: ${NETBSDSRCDIR}/usr.bin/ktrace ${NETBSDSRCDIR}/usr.bin/kdump
