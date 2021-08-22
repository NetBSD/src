#	$NetBSD: Makefile,v 1.35 2021/08/22 22:24:12 rillig Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/6/93

.include <bsd.own.mk>		# for MKDYNAMICROOT & NETBSDSRCDIR

USE_FORT?= yes	# cryptographic software

PROG=	kdump
LINKS=	${BINDIR}/kdump ${BINDIR}/ioctlprint
MAN=	kdump.1 ioctlprint.1
SRCS=	kdump.c subr.c setemul.c siginfo.c
.PATH:	${NETBSDSRCDIR}/usr.bin/ktrace
CLEANFILES+=siginfo.c
LINT_SUPPORTED=	no	# kdump-ioctl.c defines uio_t twice.

.if (${MKDYNAMICROOT} == "no")
LDSTATIC?=-static
.endif

.include "Makefile.ioctl-c"
.include "Makefile.siginfo-c"

CPPFLAGS+=-I${NETBSDSRCDIR}/usr.bin/ktrace -I${NETBSDSRCDIR}/sys -D_KERNTYPES

LDADD+=-lutil
DPADD+=${LIBUTIL}

CWARNFLAGS.gcc+=	${GCC_NO_ADDR_OF_PACKED_MEMBER}

.include <bsd.prog.mk>
