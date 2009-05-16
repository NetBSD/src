#	$NetBSD: Makefile,v 1.24 2009/05/16 21:37:48 christos Exp $

NOMAN=		# defined

.include <bsd.own.mk>		# for MKDYNAMICROOT & NETBSDSRCDIR

PROG=		ktruss
SRCS=		ktrace.c dump.c subr.c misc.c setemul.c
CPPFLAGS+=	-I. -I${NETBSDSRCDIR}/usr.bin/ktrace -I${NETBSDSRCDIR}/sys \
		-I${NETBSDSRCDIR}/usr.bin/kdump -DKTRUSS
CLEANFILES+=	misc.c misc.h

.if (${MKDYNAMICROOT} == "no")
LDSTATIC?=-static
.endif

dump.c misc.c: misc.h
misc.h misc.c dump.c: ${DESTDIR}/usr/include/sys/errno.h makeerrnos.sh \
	       ${DESTDIR}/usr/include/sys/signal.h
	${_MKTARGET_CREATE}
	AWK=${TOOL_AWK:Q} CPP=${CPP:Q} CPPFLAGS=${CPPFLAGS:Q} \
	    ${HOST_SH} ${.CURDIR}/makeerrnos.sh \
	    ${DESTDIR}/usr/include/sys/errno.h \
	    ${DESTDIR}/usr/include/sys/signal.h misc

.include "${NETBSDSRCDIR}/usr.bin/kdump/Makefile.ioctl-c"
.include <bsd.prog.mk>

.PATH: ${NETBSDSRCDIR}/usr.bin/ktrace ${NETBSDSRCDIR}/usr.bin/kdump
