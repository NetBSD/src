#	$NetBSD: Makefile,v 1.26 2009/12/08 02:56:37 uebayasi Exp $

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

# Hint for mkdep(1).
dump.c: misc.h

# Avoid race.
misc.c: misc.h

# Build-time generation.
misc.c misc.h: ${DESTDIR}/usr/include/sys/errno.h makeerrnos.sh \
	       ${DESTDIR}/usr/include/sys/signal.h
	${_MKTARGET_CREATE}
	AWK=${TOOL_AWK:Q} CPP=${CPP:Q} CPPFLAGS=${CPPFLAGS:Q} \
	    ${HOST_SH} ${.CURDIR}/makeerrnos.sh \
	    ${DESTDIR}/usr/include/sys/errno.h \
	    ${DESTDIR}/usr/include/sys/signal.h misc

.include "${NETBSDSRCDIR}/usr.bin/kdump/Makefile.ioctl-c"
.include <bsd.prog.mk>

.PATH: ${NETBSDSRCDIR}/usr.bin/ktrace ${NETBSDSRCDIR}/usr.bin/kdump
