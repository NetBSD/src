#	$NetBSD: Makefile,v 1.33 2023/06/03 08:53:00 lukem Exp $

NOMAN=		# defined

.include <bsd.own.mk>		# for MKDYNAMICROOT & NETBSDSRCDIR

PROG=		ktruss
SRCS=		ktrace.c dump.c subr.c misc.c setemul.c
CLEANFILES+=	misc.c misc.h
LINT_SUPPORTED=	no	# ktruss-ioctl.c defines uio_t twice.

.if (${MKDYNAMICROOT} == "no")
LDSTATIC?=-static
.endif

# Hint for mkdep(1).
dump.c: misc.h

# Avoid race.
.ORDER: misc.c misc.h

# Build-time generation.
misc.c misc.h: ${DESTDIR}/usr/include/sys/errno.h makeerrnos.sh \
	       ${DESTDIR}/usr/include/sys/signal.h
	${_MKTARGET_CREATE}
	AWK=${TOOL_AWK:Q} CPP=${CPP:Q} CPPFLAGS=${CPPFLAGS:Q} \
	    ${HOST_SH} ${.CURDIR}/makeerrnos.sh \
	    ${DESTDIR}/usr/include/sys/errno.h \
	    ${DESTDIR}/usr/include/sys/signal.h misc

.include "${NETBSDSRCDIR}/usr.bin/kdump/Makefile.ioctl-c"

CPPFLAGS+=	-I. -I${NETBSDSRCDIR}/usr.bin/ktrace -I${NETBSDSRCDIR}/sys \
		-I${NETBSDSRCDIR}/usr.bin/kdump -DKTRUSS -D_KERNTYPES

CWARNFLAGS.gcc+=	${CC_WNO_ADDRESS_OF_PACKED_MEMBER}

.include <bsd.prog.mk>

.PATH: ${NETBSDSRCDIR}/usr.bin/ktrace ${NETBSDSRCDIR}/usr.bin/kdump
